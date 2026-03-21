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
  out->name_len = parser->current.lexeme_len;
  out->is_mutable = is_mutable;
  out->has_type = 0;
  out->type_name = NULL;
  out->type_name_len = 0U;

  beet_parser_advance(parser);

  if (beet_parser_match(parser, BEET_TOKEN_KW_IS)) {
    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
      return 0;
    }

    out->has_type = 1;
    out->type_name = parser->current.lexeme;
    out->type_name_len = parser->current.lexeme_len;

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

static int beet_parser_parse_param(beet_parser *parser, beet_ast_param *out) {
  assert(parser != NULL);
  assert(out != NULL);

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  out->name = parser->current.lexeme;
  out->name_len = parser->current.lexeme_len;
  beet_parser_advance(parser);

  if (!beet_expect(parser, BEET_TOKEN_KW_IS)) {
    return 0;
  }

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  out->type_name = parser->current.lexeme;
  out->type_name_len = parser->current.lexeme_len;
  beet_parser_advance(parser);

  return 1;
}

static void beet_ast_expr_init(beet_ast_expr *expr) {
  assert(expr != NULL);

  expr->kind = BEET_AST_EXPR_INVALID;
  expr->text = NULL;
  expr->text_len = 0U;
  expr->is_resolved = 0;
  expr->resolved_depth = 0U;
  expr->resolved_is_mutable = 0;
  expr->unary_op = BEET_AST_UNARY_NEGATE;
  expr->binary_op = BEET_AST_BINARY_ADD;
  expr->left = NULL;
  expr->right = NULL;
}

static void beet_ast_stmt_init(beet_ast_stmt *stmt) {
  assert(stmt != NULL);

  stmt->kind = BEET_AST_STMT_INVALID;
  stmt->binding.name = NULL;
  stmt->binding.name_len = 0U;
  stmt->binding.is_mutable = 0;
  stmt->binding.has_type = 0;
  stmt->binding.type_name = NULL;
  stmt->binding.type_name_len = 0U;
  stmt->binding.value_text = NULL;
  stmt->binding.value_len = 0U;
  beet_ast_expr_init(&stmt->expr);
  beet_ast_expr_init(&stmt->condition);
  stmt->then_body = NULL;
  stmt->then_body_count = 0U;
  stmt->loop_body = NULL;
  stmt->loop_body_count = 0U;
}

static beet_ast_expr *beet_ast_expr_alloc(beet_ast_function *function) {
  beet_ast_expr *expr;

  assert(function != NULL);

  if (function->expr_count >= BEET_AST_MAX_EXPR_NODES) {
    return NULL;
  }

  expr = &function->expr_nodes[function->expr_count];
  function->expr_count += 1U;
  beet_ast_expr_init(expr);
  return expr;
}

static beet_ast_expr *beet_ast_expr_store(beet_ast_function *function,
                                          const beet_ast_expr *source) {
  beet_ast_expr *expr;

  assert(function != NULL);
  assert(source != NULL);

  expr = beet_ast_expr_alloc(function);
  if (expr == NULL) {
    return NULL;
  }

  *expr = *source;
  return expr;
}

static beet_ast_stmt *beet_ast_stmt_alloc(beet_ast_function *function) {
  beet_ast_stmt *stmt;

  assert(function != NULL);

  if (function->stmt_node_count >= BEET_AST_MAX_STMT_NODES) {
    return NULL;
  }

  stmt = &function->stmt_nodes[function->stmt_node_count];
  function->stmt_node_count += 1U;
  beet_ast_stmt_init(stmt);
  return stmt;
}

static int beet_parser_parse_expr(beet_parser *parser,
                                  beet_ast_function *function,
                                  beet_ast_expr *out);

static int beet_parser_parse_primary_expr(beet_parser *parser,
                                          beet_ast_function *function,
                                          beet_ast_expr *out) {
  assert(parser != NULL);
  assert(function != NULL);
  assert(out != NULL);

  beet_ast_expr_init(out);

  if (parser->current.kind == BEET_TOKEN_INT_LITERAL) {
    out->kind = BEET_AST_EXPR_INT_LITERAL;
    out->text = parser->current.lexeme;
    out->text_len = parser->current.lexeme_len;
    beet_parser_advance(parser);
    return 1;
  }

  if (parser->current.kind == BEET_TOKEN_IDENTIFIER) {
    out->kind = BEET_AST_EXPR_NAME;
    out->text = parser->current.lexeme;
    out->text_len = parser->current.lexeme_len;
    beet_parser_advance(parser);
    return 1;
  }

  if (beet_parser_match(parser, BEET_TOKEN_LPAREN)) {
    if (!beet_parser_parse_expr(parser, function, out)) {
      return 0;
    }

    return beet_expect(parser, BEET_TOKEN_RPAREN);
  }

  return 0;
}

static int beet_parser_parse_unary_expr(beet_parser *parser,
                                        beet_ast_function *function,
                                        beet_ast_expr *out) {
  beet_token operator_token;
  beet_ast_expr *operand;

  assert(parser != NULL);
  assert(function != NULL);
  assert(out != NULL);

  if (parser->current.kind != BEET_TOKEN_MINUS) {
    return beet_parser_parse_primary_expr(parser, function, out);
  }

  operator_token = parser->current;
  beet_parser_advance(parser);

  operand = beet_ast_expr_alloc(function);
  if (operand == NULL) {
    return 0;
  }

  if (!beet_parser_parse_unary_expr(parser, function, operand)) {
    return 0;
  }

  beet_ast_expr_init(out);
  out->kind = BEET_AST_EXPR_UNARY;
  out->text = operator_token.lexeme;
  out->text_len = operator_token.lexeme_len;
  out->unary_op = BEET_AST_UNARY_NEGATE;
  out->left = operand;
  return 1;
}

static int beet_parser_parse_factor_expr(beet_parser *parser,
                                         beet_ast_function *function,
                                         beet_ast_expr *out) {
  assert(parser != NULL);
  assert(function != NULL);
  assert(out != NULL);

  if (!beet_parser_parse_unary_expr(parser, function, out)) {
    return 0;
  }

  while (parser->current.kind == BEET_TOKEN_STAR ||
         parser->current.kind == BEET_TOKEN_SLASH) {
    beet_token operator_token;
    beet_ast_expr *left;
    beet_ast_expr *right;

    operator_token = parser->current;
    beet_parser_advance(parser);

    left = beet_ast_expr_store(function, out);
    if (left == NULL) {
      return 0;
    }

    right = beet_ast_expr_alloc(function);
    if (right == NULL) {
      return 0;
    }

    if (!beet_parser_parse_unary_expr(parser, function, right)) {
      return 0;
    }

    beet_ast_expr_init(out);
    out->kind = BEET_AST_EXPR_BINARY;
    out->text = operator_token.lexeme;
    out->text_len = operator_token.lexeme_len;
    out->binary_op = operator_token.kind == BEET_TOKEN_STAR
                         ? BEET_AST_BINARY_MUL
                         : BEET_AST_BINARY_DIV;
    out->left = left;
    out->right = right;
  }

  return 1;
}

static int beet_parser_parse_expr(beet_parser *parser,
                                  beet_ast_function *function,
                                  beet_ast_expr *out) {
  assert(parser != NULL);
  assert(function != NULL);
  assert(out != NULL);

  if (!beet_parser_parse_factor_expr(parser, function, out)) {
    return 0;
  }

  while (parser->current.kind == BEET_TOKEN_PLUS ||
         parser->current.kind == BEET_TOKEN_MINUS) {
    beet_token operator_token;
    beet_ast_expr *left;
    beet_ast_expr *right;

    operator_token = parser->current;
    beet_parser_advance(parser);

    left = beet_ast_expr_store(function, out);
    if (left == NULL) {
      return 0;
    }

    right = beet_ast_expr_alloc(function);
    if (right == NULL) {
      return 0;
    }

    if (!beet_parser_parse_factor_expr(parser, function, right)) {
      return 0;
    }

    beet_ast_expr_init(out);
    out->kind = BEET_AST_EXPR_BINARY;
    out->text = operator_token.lexeme;
    out->text_len = operator_token.lexeme_len;
    out->binary_op = operator_token.kind == BEET_TOKEN_PLUS
                         ? BEET_AST_BINARY_ADD
                         : BEET_AST_BINARY_SUB;
    out->left = left;
    out->right = right;
  }

  return 1;
}

static int beet_parser_parse_stmt(beet_parser *parser,
                                  beet_ast_function *function,
                                  beet_ast_stmt *out);

static int beet_parser_parse_while_stmt(beet_parser *parser,
                                        beet_ast_function *function,
                                        beet_ast_stmt *out) {
  size_t body_start;

  assert(parser != NULL);
  assert(function != NULL);
  assert(out != NULL);

  if (!beet_expect(parser, BEET_TOKEN_KW_WHILE)) {
    return 0;
  }

  out->kind = BEET_AST_STMT_WHILE;
  if (!beet_parser_parse_expr(parser, function, &out->condition)) {
    return 0;
  }

  if (!beet_expect(parser, BEET_TOKEN_LBRACE)) {
    return 0;
  }

  body_start = function->stmt_node_count;
  while (parser->current.kind != BEET_TOKEN_RBRACE) {
    beet_ast_stmt *nested_stmt;

    nested_stmt = beet_ast_stmt_alloc(function);
    if (nested_stmt == NULL) {
      return 0;
    }

    if (!beet_parser_parse_stmt(parser, function, nested_stmt)) {
      return 0;
    }

    out->loop_body_count += 1U;
  }

  if (!beet_expect(parser, BEET_TOKEN_RBRACE)) {
    return 0;
  }

  if (out->loop_body_count > 0U) {
    out->loop_body = &function->stmt_nodes[body_start];
  }

  return 1;
}

static int beet_parser_parse_if_stmt(beet_parser *parser,
                                     beet_ast_function *function,
                                     beet_ast_stmt *out) {
  size_t body_start;

  assert(parser != NULL);
  assert(function != NULL);
  assert(out != NULL);

  if (!beet_expect(parser, BEET_TOKEN_KW_IF)) {
    return 0;
  }

  out->kind = BEET_AST_STMT_IF;
  if (!beet_parser_parse_expr(parser, function, &out->condition)) {
    return 0;
  }

  if (!beet_expect(parser, BEET_TOKEN_LBRACE)) {
    return 0;
  }

  body_start = function->stmt_node_count;
  while (parser->current.kind != BEET_TOKEN_RBRACE) {
    beet_ast_stmt *nested_stmt;

    nested_stmt = beet_ast_stmt_alloc(function);
    if (nested_stmt == NULL) {
      return 0;
    }

    if (!beet_parser_parse_stmt(parser, function, nested_stmt)) {
      return 0;
    }

    out->then_body_count += 1U;
  }

  if (!beet_expect(parser, BEET_TOKEN_RBRACE)) {
    return 0;
  }

  if (out->then_body_count > 0U) {
    out->then_body = &function->stmt_nodes[body_start];
  }

  return 1;
}

static int beet_parser_parse_stmt(beet_parser *parser,
                                  beet_ast_function *function,
                                  beet_ast_stmt *out) {
  assert(parser != NULL);
  assert(function != NULL);
  assert(out != NULL);

  beet_ast_stmt_init(out);

  if (parser->current.kind == BEET_TOKEN_KW_BIND ||
      parser->current.kind == BEET_TOKEN_KW_MUTABLE) {
    out->kind = BEET_AST_STMT_BINDING;
    return beet_parser_parse_binding(parser, &out->binding);
  }

  if (parser->current.kind == BEET_TOKEN_KW_RETURN) {
    out->kind = BEET_AST_STMT_RETURN;
    beet_parser_advance(parser);
    return beet_parser_parse_expr(parser, function, &out->expr);
  }

  if (parser->current.kind == BEET_TOKEN_KW_IF) {
    return beet_parser_parse_if_stmt(parser, function, out);
  }

  if (parser->current.kind == BEET_TOKEN_KW_WHILE) {
    return beet_parser_parse_while_stmt(parser, function, out);
  }

  out->kind = BEET_AST_STMT_INVALID;
  return 0;
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
  out->name_len = parser->current.lexeme_len;
  out->return_type_name = NULL;
  out->return_type_name_len = 0U;
  out->param_count = 0U;
  out->body_count = 0U;
  out->stmt_node_count = 0U;
  out->expr_count = 0U;

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
  out->return_type_name_len = parser->current.lexeme_len;
  beet_parser_advance(parser);

  if (!beet_expect(parser, BEET_TOKEN_LBRACE)) {
    return 0;
  }

  while (parser->current.kind != BEET_TOKEN_RBRACE) {
    if (out->body_count >= BEET_AST_MAX_BODY_STMTS) {
      return 0;
    }

    if (!beet_parser_parse_stmt(parser, out, &out->body[out->body_count])) {
      return 0;
    }

    out->body_count += 1U;
  }

  if (!beet_expect(parser, BEET_TOKEN_RBRACE)) {
    return 0;
  }

  return 1;
}

static int beet_parser_parse_field(beet_parser *parser, beet_ast_field *out) {
  assert(parser != NULL);
  assert(out != NULL);

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  out->name = parser->current.lexeme;
  out->name_len = parser->current.lexeme_len;
  beet_parser_advance(parser);

  if (!beet_expect(parser, BEET_TOKEN_KW_IS)) {
    return 0;
  }

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  out->type_name = parser->current.lexeme;
  out->type_name_len = parser->current.lexeme_len;
  beet_parser_advance(parser);

  return 1;
}

static int beet_parser_parse_choice_variant(beet_parser *parser,
                                            beet_ast_choice_variant *out) {
  assert(parser != NULL);
  assert(out != NULL);

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  out->name = parser->current.lexeme;
  out->name_len = parser->current.lexeme_len;
  out->has_payload = 0;
  out->payload_type_name = NULL;
  out->payload_type_name_len = 0U;
  beet_parser_advance(parser);

  if (!beet_parser_match(parser, BEET_TOKEN_LPAREN)) {
    return 1;
  }

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  out->has_payload = 1;
  out->payload_type_name = parser->current.lexeme;
  out->payload_type_name_len = parser->current.lexeme_len;
  beet_parser_advance(parser);

  if (!beet_expect(parser, BEET_TOKEN_RPAREN)) {
    return 0;
  }

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
  out->name_len = parser->current.lexeme_len;
  out->is_structure = 0;
  out->is_choice = 0;
  out->field_count = 0U;
  out->variant_count = 0U;

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

  if (beet_parser_match(parser, BEET_TOKEN_KW_STRUCTURE)) {
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

  if (!beet_parser_match(parser, BEET_TOKEN_KW_CHOICE)) {
    return 0;
  }

  out->is_choice = 1;

  if (!beet_expect(parser, BEET_TOKEN_LBRACE)) {
    return 0;
  }

  while (parser->current.kind != BEET_TOKEN_RBRACE) {
    if (out->variant_count >= BEET_AST_MAX_VARIANTS) {
      return 0;
    }

    if (!beet_parser_parse_choice_variant(parser,
                                          &out->variants[out->variant_count])) {
      return 0;
    }

    out->variant_count += 1U;
  }

  if (!beet_expect(parser, BEET_TOKEN_RBRACE)) {
    return 0;
  }

  return 1;
}
