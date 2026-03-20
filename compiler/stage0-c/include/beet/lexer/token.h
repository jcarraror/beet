#ifndef BEET_LEXER_TOKEN_H
#define BEET_LEXER_TOKEN_H

#include <stddef.h>

#include "beet/support/source.h"

typedef enum beet_token_kind {
    BEET_TOKEN_ERROR = 0,
    BEET_TOKEN_EOF,

    BEET_TOKEN_IDENTIFIER,
    BEET_TOKEN_INT_LITERAL,

    BEET_TOKEN_KW_FN,
    BEET_TOKEN_KW_LET,
    BEET_TOKEN_KW_MODULE,
    BEET_TOKEN_KW_IF,
    BEET_TOKEN_KW_THEN,
    BEET_TOKEN_KW_ELSE,
    BEET_TOKEN_KW_WHILE,
    BEET_TOKEN_KW_MUT,
    BEET_TOKEN_KW_STRUCT,
    BEET_TOKEN_KW_ENUM,

    BEET_TOKEN_LPAREN,
    BEET_TOKEN_RPAREN,
    BEET_TOKEN_LBRACE,
    BEET_TOKEN_RBRACE,
    BEET_TOKEN_COMMA,
    BEET_TOKEN_COLON,
    BEET_TOKEN_DOT,
    BEET_TOKEN_PLUS,
    BEET_TOKEN_MINUS,
    BEET_TOKEN_STAR,
    BEET_TOKEN_SLASH,
    BEET_TOKEN_EQUAL,

    BEET_TOKEN_ARROW
} beet_token_kind;

typedef struct beet_token {
    beet_token_kind kind;
    beet_source_span span;
    const char *lexeme;
    size_t lexeme_len;
} beet_token;

const char *beet_token_kind_name(beet_token_kind kind);

#endif