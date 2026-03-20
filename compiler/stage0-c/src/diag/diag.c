#include "beet/diag/diag.h"

#include <assert.h>
#include <stddef.h>

#define BEET_COLOR_RESET "\x1b[0m"
#define BEET_COLOR_RED "\x1b[31m"
#define BEET_COLOR_YELLOW "\x1b[33m"
#define BEET_COLOR_BLUE "\x1b[34m"

static const char *beet_level_name(beet_diag_level level) {
  switch (level) {
  case BEET_DIAG_NOTE:
    return "note";
  case BEET_DIAG_WARNING:
    return "warning";
  case BEET_DIAG_ERROR:
    return "error";
  }
  return "unknown";
}

static const char *beet_level_color(beet_diag_level level) {
  switch (level) {
  case BEET_DIAG_NOTE:
    return BEET_COLOR_BLUE;
  case BEET_DIAG_WARNING:
    return BEET_COLOR_YELLOW;
  case BEET_DIAG_ERROR:
    return BEET_COLOR_RED;
  }
  return "";
}

static void beet_print_colored(FILE *stream, bool use_color, const char *color,
                               const char *text) {
  if (use_color) {
    fputs(color, stream);
    fputs(text, stream);
    fputs(BEET_COLOR_RESET, stream);
  } else {
    fputs(text, stream);
  }
}

static void beet_print_repeat(FILE *stream, char ch, unsigned count) {
  unsigned i;
  for (i = 0U; i < count; ++i) {
    fputc((int)ch, stream);
  }
}

static void beet_emit_header(FILE *stream, const beet_diag *diag,
                             beet_diag_options options) {
  beet_print_colored(stream, options.use_color, beet_level_color(diag->level),
                     beet_level_name(diag->level));
  fputs(": ", stream);
  fputs(diag->message, stream);
  fputc('\n', stream);

  fprintf(stream, "  --> %s:%u:%u\n", diag->file->path, diag->span.start.line,
          diag->span.start.column);
}

static void beet_emit_source_line(FILE *stream, const beet_diag *diag,
                                  beet_diag_options options) {
  const char *line_text;
  size_t line_len;
  unsigned underline_column;
  unsigned underline_width;
  unsigned line_no;

  line_no = diag->span.start.line;
  line_text = beet_source_line_text(diag->file, line_no, &line_len);
  assert(line_text != NULL);

  fprintf(stream, "   |\n");
  fprintf(stream, "%2u | ", line_no);
  fwrite(line_text, 1U, line_len, stream);
  fputc('\n', stream);

  underline_column = diag->span.start.column;
  if (diag->span.end.line == diag->span.start.line &&
      diag->span.end.column > diag->span.start.column) {
    underline_width = diag->span.end.column - diag->span.start.column;
  } else {
    underline_width = 1U;
  }

  fprintf(stream, "   | ");
  if (underline_column > 1U) {
    beet_print_repeat(stream, ' ', underline_column - 1U);
  }

  if (options.use_color) {
    fputs(beet_level_color(diag->level), stream);
    beet_print_repeat(stream, '^', underline_width);
    fputs(BEET_COLOR_RESET, stream);
  } else {
    beet_print_repeat(stream, '^', underline_width);
  }

  fputc('\n', stream);
}

static void beet_emit_note(FILE *stream, const beet_diag *diag,
                           beet_diag_options options) {
  if (diag->note == NULL || diag->note[0] == '\0') {
    return;
  }

  fputs("   = ", stream);
  beet_print_colored(stream, options.use_color, BEET_COLOR_BLUE, "note");
  fputs(": ", stream);
  fputs(diag->note, stream);
  fputc('\n', stream);
}

void beet_diag_emit(FILE *stream, const beet_diag *diag,
                    beet_diag_options options) {
  assert(stream != NULL);
  assert(diag != NULL);
  assert(diag->file != NULL);
  assert(diag->file->path != NULL);
  assert(diag->file->text != NULL);
  assert(diag->message != NULL);

  beet_emit_header(stream, diag, options);
  beet_emit_source_line(stream, diag, options);
  beet_emit_note(stream, diag, options);
}