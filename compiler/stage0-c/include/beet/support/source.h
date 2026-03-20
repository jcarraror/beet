#ifndef BEET_SUPPORT_SOURCE_H
#define BEET_SUPPORT_SOURCE_H

#include <stddef.h>

typedef struct beet_source_file {
    const char *path;
    const char *text;
    size_t text_len;
} beet_source_file; // Opaque type representing a source file

typedef struct beet_source_pos {
    size_t offset;
    unsigned line;
    unsigned column;
} beet_source_pos; // Represents a position in the source file

typedef struct beet_source_span {
    beet_source_pos start;
    beet_source_pos end;
} beet_source_span; // Represents a span in the source file

// Function declarations for working with source files and positions
beet_source_file beet_source_file_from_cstr(const char *path, const char *text);


beet_source_pos beet_source_pos_at(
    const beet_source_file *file,
    size_t offset
); // Converts a byte offset in the source file to a line and column position

beet_source_span beet_source_span_from_offsets(
    const beet_source_file *file,
    size_t start_offset,
    size_t end_offset
); // Converts a range of byte offsets in the source file to a source span

#endif