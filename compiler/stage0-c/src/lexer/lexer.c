#include "beet/lexer/lexer.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static bool beet_is_alpha(char ch) {
    return ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            ch == '_');
}

static bool beet_is_digit(char ch) {
    return (ch >= '0' && ch <= '9');
}

static bool beet_is_alnum(char ch) {
    return beet_is_alpha(ch) || beet_is_digit(ch);
}

static bool beet_is_space(char ch) {
    return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

static char beet_peek(const beet_lexer *lexer) {
    assert(lexer != NULL);
    assert(lexer->file != NULL);
    assert(lexer->file->text != NULL);

    if (lexer->offset >= lexer->file->text_len) {
        return '\0';
    }
    return lexer->file->text[lexer->offset];
}

static char beet_peek_next(const beet_lexer *lexer) {
    size_t next_offset;

    assert(lexer != NULL);
    assert(lexer->file != NULL);
    assert(lexer->file->text != NULL);

    next_offset = lexer->offset + 1U;
    if (next_offset >= lexer->file->text_len) {
        return '\0';
    }
    return lexer->file->text[next_offset];
}

static char beet_advance(beet_lexer *lexer) {
    char ch;

    assert(lexer != NULL);

    ch = beet_peek(lexer);
    if (ch != '\0') {
        lexer->offset += 1U;
    }
    return ch;
}

static void beet_skip_whitespace(beet_lexer *lexer) {
    while (beet_is_space(beet_peek(lexer))) {
        (void)beet_advance(lexer);
    }
}

static beet_token beet_make_token(
    const beet_lexer *lexer,
    beet_token_kind kind,
    size_t start,
    size_t end
) {
    beet_token token;

    assert(lexer != NULL);
    assert(lexer->file != NULL);
    assert(start <= end);
    assert(end <= lexer->file->text_len);

    token.kind = kind;
    token.span = beet_source_span_from_offsets(lexer->file, start, end);
    token.lexeme = lexer->file->text + start;
    token.lexeme_len = end - start;
    return token;
}

static beet_token_kind beet_identifier_kind(const char *text, size_t len) {
    if (len == 2U && strncmp(text, "fn", 2U) == 0) {
        return BEET_TOKEN_KW_FN;
    }
    if (len == 3U && strncmp(text, "let", 3U) == 0) {
        return BEET_TOKEN_KW_LET;
    }
    if (len == 6U && strncmp(text, "module", 6U) == 0) {
        return BEET_TOKEN_KW_MODULE;
    }
    if (len == 2U && strncmp(text, "if", 2U) == 0) {
        return BEET_TOKEN_KW_IF;
    }
    if (len == 4U && strncmp(text, "then", 4U) == 0) {
        return BEET_TOKEN_KW_THEN;
    }
    if (len == 4U && strncmp(text, "else", 4U) == 0) {
        return BEET_TOKEN_KW_ELSE;
    }
    if (len == 5U && strncmp(text, "while", 5U) == 0) {
        return BEET_TOKEN_KW_WHILE;
    }
    if (len == 3U && strncmp(text, "mut", 3U) == 0) {
        return BEET_TOKEN_KW_MUT;
    }
    if (len == 6U && strncmp(text, "struct", 6U) == 0) {
        return BEET_TOKEN_KW_STRUCT;
    }
    if (len == 4U && strncmp(text, "enum", 4U) == 0) {
        return BEET_TOKEN_KW_ENUM;
    }

    return BEET_TOKEN_IDENTIFIER;
}

static beet_token beet_scan_identifier(beet_lexer *lexer, size_t start) {
    while (beet_is_alnum(beet_peek(lexer))) {
        (void)beet_advance(lexer);
    }

    return beet_make_token(
        lexer,
        beet_identifier_kind(
            lexer->file->text + start,
            lexer->offset - start
        ),
        start,
        lexer->offset
    );
}

static beet_token beet_scan_int(beet_lexer *lexer, size_t start) {
    while (beet_is_digit(beet_peek(lexer))) {
        (void)beet_advance(lexer);
    }

    return beet_make_token(lexer, BEET_TOKEN_INT_LITERAL, start, lexer->offset);
}

const char *beet_token_kind_name(beet_token_kind kind) {
    switch (kind) {
        case BEET_TOKEN_ERROR: return "error";
        case BEET_TOKEN_EOF: return "eof";
        case BEET_TOKEN_IDENTIFIER: return "identifier";
        case BEET_TOKEN_INT_LITERAL: return "int_literal";
        case BEET_TOKEN_KW_FN: return "kw_fn";
        case BEET_TOKEN_KW_LET: return "kw_let";
        case BEET_TOKEN_KW_MODULE: return "kw_module";
        case BEET_TOKEN_KW_IF: return "kw_if";
        case BEET_TOKEN_KW_THEN: return "kw_then";
        case BEET_TOKEN_KW_ELSE: return "kw_else";
        case BEET_TOKEN_KW_WHILE: return "kw_while";
        case BEET_TOKEN_KW_MUT: return "kw_mut";
        case BEET_TOKEN_KW_STRUCT: return "kw_struct";
        case BEET_TOKEN_KW_ENUM: return "kw_enum";
        case BEET_TOKEN_LPAREN: return "lparen";
        case BEET_TOKEN_RPAREN: return "rparen";
        case BEET_TOKEN_LBRACE: return "lbrace";
        case BEET_TOKEN_RBRACE: return "rbrace";
        case BEET_TOKEN_COMMA: return "comma";
        case BEET_TOKEN_COLON: return "colon";
        case BEET_TOKEN_DOT: return "dot";
        case BEET_TOKEN_PLUS: return "plus";
        case BEET_TOKEN_MINUS: return "minus";
        case BEET_TOKEN_STAR: return "star";
        case BEET_TOKEN_SLASH: return "slash";
        case BEET_TOKEN_EQUAL: return "equal";
        case BEET_TOKEN_ARROW: return "arrow";
    }

    return "unknown";
}

void beet_lexer_init(beet_lexer *lexer, const beet_source_file *file) {
    assert(lexer != NULL);
    assert(file != NULL);
    assert(file->text != NULL);

    lexer->file = file;
    lexer->offset = 0U;
}

beet_token beet_lexer_next(beet_lexer *lexer) {
    size_t start;
    char ch;

    assert(lexer != NULL);
    assert(lexer->file != NULL);
    assert(lexer->file->text != NULL);

    beet_skip_whitespace(lexer);

    start = lexer->offset;
    ch = beet_peek(lexer);

    if (ch == '\0') {
        return beet_make_token(lexer, BEET_TOKEN_EOF, start, start);
    }

    if (beet_is_alpha(ch)) {
        (void)beet_advance(lexer);
        return beet_scan_identifier(lexer, start);
    }

    if (beet_is_digit(ch)) {
        (void)beet_advance(lexer);
        return beet_scan_int(lexer, start);
    }

    if (ch == '-' && beet_peek_next(lexer) == '>') {
        (void)beet_advance(lexer);
        (void)beet_advance(lexer);
        return beet_make_token(lexer, BEET_TOKEN_ARROW, start, lexer->offset);
    }

    (void)beet_advance(lexer);

    switch (ch) {
        case '(':
            return beet_make_token(lexer, BEET_TOKEN_LPAREN, start, lexer->offset);
        case ')':
            return beet_make_token(lexer, BEET_TOKEN_RPAREN, start, lexer->offset);
        case '{':
            return beet_make_token(lexer, BEET_TOKEN_LBRACE, start, lexer->offset);
        case '}':
            return beet_make_token(lexer, BEET_TOKEN_RBRACE, start, lexer->offset);
        case ',':
            return beet_make_token(lexer, BEET_TOKEN_COMMA, start, lexer->offset);
        case ':':
            return beet_make_token(lexer, BEET_TOKEN_COLON, start, lexer->offset);
        case '.':
            return beet_make_token(lexer, BEET_TOKEN_DOT, start, lexer->offset);
        case '+':
            return beet_make_token(lexer, BEET_TOKEN_PLUS, start, lexer->offset);
        case '-':
            return beet_make_token(lexer, BEET_TOKEN_MINUS, start, lexer->offset);
        case '*':
            return beet_make_token(lexer, BEET_TOKEN_STAR, start, lexer->offset);
        case '/':
            return beet_make_token(lexer, BEET_TOKEN_SLASH, start, lexer->offset);
        case '=':
            return beet_make_token(lexer, BEET_TOKEN_EQUAL, start, lexer->offset);
        default:
            return beet_make_token(lexer, BEET_TOKEN_ERROR, start, lexer->offset);
    }
}