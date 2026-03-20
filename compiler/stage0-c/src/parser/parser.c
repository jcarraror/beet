#include "beet/parser/parser.h"

#include <assert.h>
#include <string.h>

static void beet_parser_advance(beet_parser *parser) {
    parser->current = beet_lexer_next(&parser->lexer);
}

static int beet_parser_match(beet_parser *parser, beet_token_kind kind) {
    if (parser->current.kind == kind) {
        beet_parser_advance(parser);
        return 1;
    }
    return 0;
}

void beet_parser_init(beet_parser *parser, const beet_source_file *file) {
    assert(parser != NULL);
    assert(file != NULL);

    beet_lexer_init(&parser->lexer, file);
    beet_parser_advance(parser);
}

static int beet_expect(beet_parser *parser, beet_token_kind kind) {
    if (parser->current.kind != kind) {
        return 0;
    }
    beet_parser_advance(parser);
    return 1;
}

int beet_parser_parse_binding(beet_parser *parser, beet_ast_binding *out) {
    int is_mutable = 0;

    assert(parser != NULL);
    assert(out != NULL);

    if (parser->current.kind == BEET_TOKEN_KW_BIND) {
        is_mutable = 0;
    } else if (parser->current.kind == BEET_TOKEN_KW_MUTABLE) {
        is_mutable = 1;
    } else {
        return 0;
    }

    beet_parser_advance(parser);

    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
        return 0;
    }

    out->name = parser->current.lexeme;
    out->is_mutable = is_mutable;
    out->has_type = 0;
    out->type_name = NULL;

    beet_parser_advance(parser);

    if (beet_parser_match(parser, BEET_TOKEN_KW_IS)) {
        if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
            return 0;
        }

        out->has_type = 1;
        out->type_name = parser->current.lexeme;

        beet_parser_advance(parser);
    }

    if (!beet_expect(parser, BEET_TOKEN_EQUAL)) {
        return 0;
    }

    /* capture value as raw span (single token for now) */
    if (parser->current.kind == BEET_TOKEN_EOF) {
        return 0;
    }

    out->value_text = parser->current.lexeme;
    out->value_len = parser->current.lexeme_len;

    beet_parser_advance(parser);

    return 1;
}