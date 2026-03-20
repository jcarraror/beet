#include "beet/diag/diag.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#define BEET_COLOR_RESET   "\x1b[0m"
#define BEET_COLOR_RED     "\x1b[31m"
#define BEET_COLOR_YELLOW  "\x1b[33m"
#define BEET_COLOR_BLUE    "\x1b[34m"
#define BEET_COLOR_BOLD    "\x1b[1m"

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

static void beet_print_colored(
    FILE *stream,
    bool use_color,
    const char *color,
    const char *text
) {
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
    for (i = 0; i < count; ++i) {
        fputc((int)ch, stream);
    }
}

static void beet_find_line_bounds(
    const beet_source_file *file,
    unsigned target_line,
    size_t *line_start,
    size_t *line_end
) {
    size_t i;
    unsigned line;

    assert(file != NULL);
    assert(line_start != NULL);
    assert(line_end != NULL);

    *line_start = 0U;
    *line_end = file->text_len;

    line = 1U;
    // Find the start of the target line
    for (i = 0; i < file->text_len; ++i) {
        if (line == target_line) {
            *line_start = i;
            break;
        }
        // Count lines by looking for newline characters
        if (file->text[i] == '\n') {
            line += 1U;
        }
    }

    // Find the end of the target line
    for (i = *line_start; i < file->text_len; ++i) {
        if (file->text[i] == '\n') {
            *line_end = i;
            return;
        }
    }

    *line_end = file->text_len;
}

static void beet_emit_header(
    FILE *stream,
    const beet_diag *diag,
    beet_diag_options options
) {
    const char *level_text = beet_level_name(diag->level);
    const char *level_color = beet_level_color(diag->level);

    beet_print_colored(stream, options.use_color, BEET_COLOR_BOLD, "");
    beet_print_colored(stream, options.use_color, level_color, level_text);
    fputs(": ", stream);
    fputs(diag->message, stream);
    fputc('\n', stream);

    fprintf(
        stream,
        "  --> %s:%u:%u\n",
        diag->file->path,
        diag->span.start.line,
        diag->span.start.column
    );
}

static void beet_emit_source_line(
    FILE *stream,
    const beet_diag *diag,
    beet_diag_options options
) {
    size_t line_start;
    size_t line_end;
    size_t i;
    unsigned underline_column;
    unsigned underline_width;
    unsigned line_no = diag->span.start.line;

    beet_find_line_bounds(diag->file, line_no, &line_start, &line_end);

    fprintf(stream, "   |\n");
    fprintf(stream, "%2u | ", line_no);

    for (i = line_start; i < line_end; ++i) {
        fputc((int)diag->file->text[i], stream);
    }
    fputc('\n', stream);

    underline_column = diag->span.start.column;

    // Determine the width of the underline
    // If the span covers multiple columns on the same line, underline all of them
    // Otherwise, underline at least one character
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

static void beet_emit_note(
    FILE *stream,
    const beet_diag *diag,
    beet_diag_options options
) {
    if (diag->note == NULL || diag->note[0] == '\0') {
        return;
    }

    fputs("   = ", stream);
    beet_print_colored(stream, options.use_color, BEET_COLOR_BLUE, "note");
    fputs(": ", stream);
    fputs(diag->note, stream);
    fputc('\n', stream);
}

void beet_diag_emit(
    FILE *stream,
    const beet_diag *diag,
    beet_diag_options options
) {
    assert(stream != NULL);
    assert(diag != NULL);
    assert(diag->file != NULL);
    assert(diag->message != NULL);

    beet_emit_header(stream, diag, options);
    beet_emit_source_line(stream, diag, options);
    beet_emit_note(stream, diag, options);
}