#ifndef BEET_LEXER_LEXER_H
#define BEET_LEXER_LEXER_H

#include "beet/lexer/token.h"
#include "beet/support/source.h"

typedef struct beet_lexer {
    const beet_source_file *file;
    size_t offset;
} beet_lexer;

void beet_lexer_init(beet_lexer *lexer, const beet_source_file *file);
beet_token beet_lexer_next(beet_lexer *lexer);

#endif