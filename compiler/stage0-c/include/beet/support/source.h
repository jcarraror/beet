#ifndef BEET_SUPPORT_SOURCE_H
#define BEET_SUPPORT_SOURCE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct beet_source_pos {
    size_t offset;
    unsigned line;
    unsigned column;
} beet_source_pos; // Represents a position in the source file

typedef struct beet_source_span {
    beet_source_pos start;
    beet_source_pos end;
} beet_source_span; // Represents a span in the source file

typedef struct beet_source_file {
    char *path;
    char *text;
    size_t text_len;

    size_t *line_starts;
    size_t line_count;
} beet_source_file; // Represents a source file

void beet_source_file_init(beet_source_file *file);
void beet_source_file_dispose(beet_source_file *file);

bool beet_source_file_set_text_copy(
    beet_source_file *file,
    const char *path,
    const char *text
); // Sets the text of the source file by copying the provided string, and initializes line start information

bool beet_source_file_load(
    beet_source_file *file,
    const char *path
); // Loads the contents of a file from disk into the source file structure, and initializes line start information

beet_source_pos beet_source_pos_at(
    const beet_source_file *file,
    size_t offset
); // Converts a byte offset in the source file to a line and column position

beet_source_span beet_source_span_from_offsets(
    const beet_source_file *file,
    size_t start_offset,
    size_t end_offset
); // Converts a range of byte offsets in the source file to a source span

const char *beet_source_line_text(
    const beet_source_file *file,
    unsigned line,
    size_t *line_len
); // Retrieves the text of a specific line in the source file, along with its length

#endif