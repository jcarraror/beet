#include "beet/parser/parser.h"

#include <assert.h>

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

static int beet_expect(beet_parser *parser, beet_token_kind kind) {
    if (parser->current.kind != kind) {
        return 0;
    }

    beet_parser_advance(parser);
    return 1;
}

void beet_parser_init(beet_parser *parser, const beet_source_file *file) {
    assert(parser != NULL);
    assert(file != NULL);

    beet_lexer_init(&parser->lexer, file);
    beet_parser_advance(parser);
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

    if (parser->current.kind == BEET_TOKEN_EOF) {
        return 0;
    }

    out->value_text = parser->current.lexeme;
    out->value_len = parser->current.lexeme_len;

    beet_parser_advance(parser);

    return 1;
}

static int beet_parser_parse_param(
    beet_parser *parser,
    beet_ast_param *out
) {
    assert(parser != NULL);
    assert(out != NULL);

    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
        return 0;
    }

    out->name = parser->current.lexeme;
    beet_parser_advance(parser);

    if (!beet_expect(parser, BEET_TOKEN_KW_IS)) {
        return 0;
    }

    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
        return 0;
    }

    out->type_name = parser->current.lexeme;
    beet_parser_advance(parser);

    return 1;
}

static int beet_parser_skip_block(beet_parser *parser) {
    int depth = 1;

    assert(parser != NULL);

    while (depth > 0) {
        if (parser->current.kind == BEET_TOKEN_EOF) {
            return 0;
        }

        if (parser->current.kind == BEET_TOKEN_LBRACE) {
            depth += 1;
        } else if (parser->current.kind == BEET_TOKEN_RBRACE) {
            depth -= 1;
        }

        beet_parser_advance(parser);
    }

    return 1;
}

int beet_parser_parse_function(beet_parser *parser, beet_ast_function *out) {
    assert(parser != NULL);
    assert(out != NULL);

    if (!beet_expect(parser, BEET_TOKEN_KW_FUNCTION)) {
        return 0;
    }

    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
        return 0;
    }

    out->name = parser->current.lexeme;
    out->return_type_name = NULL;
    out->param_count = 0U;

    beet_parser_advance(parser);

    if (!beet_expect(parser, BEET_TOKEN_LPAREN)) {
        return 0;
    }

    if (parser->current.kind != BEET_TOKEN_RPAREN) {
        for (;;) {
            if (out->param_count >= BEET_AST_MAX_PARAMS) {
                return 0;
            }

            if (!beet_parser_parse_param(parser, &out->params[out->param_count])) {
                return 0;
            }

            out->param_count += 1U;

            if (beet_parser_match(parser, BEET_TOKEN_COMMA)) {
                continue;
            }

            break;
        }
    }

    if (!beet_expect(parser, BEET_TOKEN_RPAREN)) {
        return 0;
    }

    if (!beet_expect(parser, BEET_TOKEN_KW_RETURNS)) {
        return 0;
    }

    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
        return 0;
    }

    out->return_type_name = parser->current.lexeme;
    beet_parser_advance(parser);

    if (!beet_expect(parser, BEET_TOKEN_LBRACE)) {
        return 0;
    }

    if (!beet_parser_skip_block(parser)) {
        return 0;
    }

    return 1;
}

static int beet_parser_parse_field(
    beet_parser *parser,
    beet_ast_field *out
) {
    assert(parser != NULL);
    assert(out != NULL);

    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
        return 0;
    }

    out->name = parser->current.lexeme;
    beet_parser_advance(parser);

    if (!beet_expect(parser, BEET_TOKEN_KW_IS)) {
        return 0;
    }

    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
        return 0;
    }

    out->type_name = parser->current.lexeme;
    beet_parser_advance(parser);

    return 1;
}

int beet_parser_parse_type_decl(beet_parser *parser, beet_ast_type_decl *out) {
    assert(parser != NULL);
    assert(out != NULL);

    if (!beet_expect(parser, BEET_TOKEN_KW_TYPE)) {
        return 0;
    }

    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
        return 0;
    }

    out->name = parser->current.lexeme;
    out->is_structure = 0;
    out->field_count = 0U;

    beet_parser_advance(parser);

    if (beet_parser_match(parser, BEET_TOKEN_LPAREN)) {
        while (parser->current.kind != BEET_TOKEN_RPAREN) {
            if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
                return 0;
            }

            beet_parser_advance(parser);

            if (beet_parser_match(parser, BEET_TOKEN_COMMA)) {
                continue;
            }
        }

        if (!beet_expect(parser, BEET_TOKEN_RPAREN)) {
            return 0;
        }
    }

    if (!beet_expect(parser, BEET_TOKEN_EQUAL)) {
        return 0;
    }

    if (!beet_expect(parser, BEET_TOKEN_KW_STRUCTURE)) {
        return 0;
    }

    out->is_structure = 1;

    if (!beet_expect(parser, BEET_TOKEN_LBRACE)) {
        return 0;
    }

    while (parser->current.kind != BEET_TOKEN_RBRACE) {
        if (out->field_count >= BEET_AST_MAX_FIELDS) {
            return 0;
        }

        if (!beet_parser_parse_field(parser, &out->fields[out->field_count])) {
            return 0;
        }

        out->field_count += 1U;
    }

    if (!beet_expect(parser, BEET_TOKEN_RBRACE)) {
        return 0;
    }

    return 1;
}