#ifndef BEET_PARSER_PARSER_H
#define BEET_PARSER_PARSER_H

#include "beet/lexer/lexer.h"
#include "beet/parser/ast.h"

typedef struct beet_parser {
    beet_lexer lexer;
    beet_token current;
} beet_parser;

void beet_parser_init(beet_parser *parser, const beet_source_file *file);

/* returns 1 if a binding was parsed, 0 otherwise */
int beet_parser_parse_binding(beet_parser *parser, beet_ast_binding *out);

/* returns 1 if a function was parsed, 0 otherwise */
int beet_parser_parse_function(beet_parser *parser, beet_ast_function *out);

#endif