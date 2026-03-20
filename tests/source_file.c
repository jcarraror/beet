#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "beet/support/source.h"

static void test_set_text_copy(void) {
  const char *text = "alpha\nbeta\ngamma";
  beet_source_file file;
  beet_source_pos pos;
  const char *line_text;
  size_t line_len;

  beet_source_file_init(&file);

  assert(beet_source_file_set_text_copy(&file, "mem.beet", text));
  assert(file.text != NULL);
  assert(file.path != NULL);
  assert(file.text_len == 16U);
  assert(file.line_count == 3U);

  pos = beet_source_pos_at(&file, 0U);
  assert(pos.line == 1U);
  assert(pos.column == 1U);

  pos = beet_source_pos_at(&file, 6U);
  assert(pos.line == 2U);
  assert(pos.column == 1U);

  pos = beet_source_pos_at(&file, 11U);
  assert(pos.line == 3U);
  assert(pos.column == 1U);

  line_text = beet_source_line_text(&file, 2U, &line_len);
  assert(line_text != NULL);
  assert(line_len == 4U);
  assert(line_text[0] == 'b');
  assert(line_text[1] == 'e');
  assert(line_text[2] == 't');
  assert(line_text[3] == 'a');

  beet_source_file_dispose(&file);
}

static void test_load_file(void) {
  const char *path = "source_file_tmp.beet";
  const char *contents = "line1\nline2\n";
  FILE *fp;
  beet_source_file file;
  beet_source_pos pos;

  fp = fopen(path, "wb");
  assert(fp != NULL);
  assert(fputs(contents, fp) >= 0);
  assert(fclose(fp) == 0);

  beet_source_file_init(&file);
  assert(beet_source_file_load(&file, path));
  assert(file.text_len == 12U);
  assert(file.line_count == 3U);

  pos = beet_source_pos_at(&file, 6U);
  assert(pos.line == 2U);
  assert(pos.column == 1U);

  beet_source_file_dispose(&file);

  assert(remove(path) == 0);
}

int main(void) {
  test_set_text_copy();
  test_load_file();
  return 0;
}