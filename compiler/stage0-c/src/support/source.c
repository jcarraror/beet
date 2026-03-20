#include "beet/support/source.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *beet_strdup_local(const char *text) {
  char *copy;
  size_t len;

  assert(text != NULL);

  len = strlen(text);
  copy = (char *)malloc(len + 1U);
  if (copy == NULL) {
    return NULL;
  }

  memcpy(copy, text, len + 1U);
  return copy;
}

static void beet_source_file_clear(beet_source_file *file) {
  assert(file != NULL);

  free(file->path);
  free(file->text);
  free(file->line_starts);

  file->path = NULL;
  file->text = NULL;
  file->text_len = 0U;
  file->line_starts = NULL;
  file->line_count = 0U;
}

static bool beet_source_build_line_index(beet_source_file *file) {
  size_t i;
  size_t count;
  size_t line_index;

  assert(file != NULL);
  assert(file->text != NULL);

  count = 1U;
  // Count the number of lines by counting newline characters
  for (i = 0; i < file->text_len; ++i) {
    if (file->text[i] == '\n') {
      count += 1U;
    }
  }

  file->line_starts = (size_t *)malloc(sizeof(size_t) * count);
  if (file->line_starts == NULL) {
    return false;
  }

  file->line_count = count;
  file->line_starts[0] = 0U;

  line_index = 1U;
  // Populate the line start offsets
  for (i = 0; i < file->text_len; ++i) {
    if (file->text[i] == '\n') {
      if (line_index < count) {
        file->line_starts[line_index] = i + 1U;
        line_index += 1U;
      }
    }
  }

  return true;
}

void beet_source_file_init(beet_source_file *file) {
  assert(file != NULL);

  file->path = NULL;
  file->text = NULL;
  file->text_len = 0U;
  file->line_starts = NULL;
  file->line_count = 0U;
}

void beet_source_file_dispose(beet_source_file *file) {
  assert(file != NULL);
  beet_source_file_clear(file);
}

bool beet_source_file_set_text_copy(beet_source_file *file, const char *path,
                                    const char *text) {
  char *path_copy;
  char *text_copy;

  assert(file != NULL);
  assert(path != NULL);
  assert(text != NULL);

  beet_source_file_clear(file);

  path_copy = beet_strdup_local(path);
  if (path_copy == NULL) {
    return false;
  }

  text_copy = beet_strdup_local(text);
  if (text_copy == NULL) {
    free(path_copy);
    return false;
  }

  file->path = path_copy;
  file->text = text_copy;
  file->text_len = strlen(text_copy);

  if (!beet_source_build_line_index(file)) {
    beet_source_file_clear(file);
    return false;
  }

  return true;
}

bool beet_source_file_load(beet_source_file *file, const char *path) {
  FILE *fp;
  long raw_size;
  size_t size;
  char *buffer;
  size_t read_count;

  assert(file != NULL);
  assert(path != NULL);

  beet_source_file_clear(file);

  fp = fopen(path, "rb");
  if (fp == NULL) {
    return false;
  }

  // Size the file by seeking to the end and getting the position
  if (fseek(fp, 0L, SEEK_END) != 0) {
    fclose(fp);
    return false;
  }

  raw_size = ftell(fp);
  if (raw_size < 0L) {
    fclose(fp);
    return false;
  }

  if (fseek(fp, 0L, SEEK_SET) != 0) {
    fclose(fp);
    return false;
  }

  size = (size_t)raw_size;
  buffer = (char *)malloc(size + 1U);
  if (buffer == NULL) {
    fclose(fp);
    return false;
  }

  read_count = fread(buffer, 1U, size, fp);
  fclose(fp);

  if (read_count != size) {
    free(buffer);
    return false;
  }

  buffer[size] = '\0';

  file->path = beet_strdup_local(path);
  if (file->path == NULL) {
    free(buffer);
    return false;
  }

  file->text = buffer;
  file->text_len = size;

  if (!beet_source_build_line_index(file)) {
    beet_source_file_clear(file);
    return false;
  }

  return true;
}

beet_source_pos beet_source_pos_at(const beet_source_file *file,
                                   size_t offset) {
  beet_source_pos pos;
  size_t low;
  size_t high;
  size_t mid;
  size_t line_index;

  assert(file != NULL);
  assert(file->text != NULL);
  assert(offset <= file->text_len);
  assert(file->line_count > 0U);

  low = 0U;
  high = file->line_count;
  line_index = 0U;

  // Binary search to find the line index corresponding to the offset
  while (low < high) {
    mid = low + (high - low) / 2U;
    if (file->line_starts[mid] <= offset) {
      line_index = mid;
      low = mid + 1U;
    } else {
      high = mid;
    }
  }

  pos.offset = offset;
  pos.line = (unsigned)(line_index + 1U);
  pos.column = (unsigned)((offset - file->line_starts[line_index]) + 1U);
  return pos;
}

beet_source_span beet_source_span_from_offsets(const beet_source_file *file,
                                               size_t start_offset,
                                               size_t end_offset) {
  beet_source_span span;

  assert(file != NULL);
  assert(start_offset <= end_offset);
  assert(end_offset <= file->text_len);

  span.start = beet_source_pos_at(file, start_offset);
  span.end = beet_source_pos_at(file, end_offset);
  return span;
}

const char *beet_source_line_text(const beet_source_file *file, unsigned line,
                                  size_t *line_len) {
  size_t index;
  size_t start;
  size_t end;

  assert(file != NULL);
  assert(file->text != NULL);
  assert(line_len != NULL);

  if (line == 0U) {
    *line_len = 0U;
    return NULL;
  }

  index = (size_t)(line - 1U);
  if (index >= file->line_count) {
    *line_len = 0U;
    return NULL;
  }

  start = file->line_starts[index];
  if (index + 1U < file->line_count) {
    end = file->line_starts[index + 1U];
    if (end > start && file->text[end - 1U] == '\n') {
      end -= 1U;
    }
  } else {
    end = file->text_len;
  }

  *line_len = end - start;
  return file->text + start;
}