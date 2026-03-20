#include "beet/support/source.h"

#include <assert.h>
#include <string.h>

static size_t beet_cstr_len(const char *text) {
    return strlen(text);
}

beet_source_file beet_source_file_from_cstr(const char *path, const char *text) {
    beet_source_file file;
    file.path = path;
    file.text = text;
    file.text_len = beet_cstr_len(text);
    return file;
}

beet_source_pos beet_source_pos_at(
    const beet_source_file *file,
    size_t offset
) {
    beet_source_pos pos;

    assert(file != NULL);
    assert(offset <= file->text_len);

    pos.offset = offset;
    pos.line = 1U;
    pos.column = 1U;

    size_t i = 0;
    for (i = 0; i < offset; ++i) {
        if (file->text[i] == '\n') {
            pos.line += 1U;
            pos.column = 1U;
        } else {
            pos.column += 1U;
        }
    }

    return pos;
}

beet_source_span beet_source_span_from_offsets(
    const beet_source_file *file,
    size_t start_offset,
    size_t end_offset
) {
    beet_source_span span;

    assert(file != NULL);
    assert(start_offset <= end_offset);
    assert(end_offset <= file->text_len);

    span.start = beet_source_pos_at(file, start_offset);
    span.end = beet_source_pos_at(file, end_offset);
    return span;
}