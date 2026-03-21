#ifndef BEET_PARSER_PARSER_H
#define BEET_PARSER_PARSER_H

#include "beet/lexer/lexer.h"
#include "beet/parser/ast.h"

typedef struct beet_parser {
  beet_lexer lexer;
  beet_token current;
  beet_ast_expr binding_expr_nodes[BEET_AST_MAX_EXPR_NODES];
  size_t binding_expr_count;
} beet_parser;

void beet_parser_init(beet_parser *parser, const beet_source_file *file);

int beet_parser_parse_binding(beet_parser *parser, beet_ast_binding *out);
int beet_parser_parse_function(beet_parser *parser, beet_ast_function *out);
int beet_parser_parse_type_decl(beet_parser *parser, beet_ast_type_decl *out);

#endif