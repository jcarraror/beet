#include "beet/parser/parser.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "beet/support/intern.h"

typedef struct beet_ast_expr_pool {
  beet_ast_expr *expr_nodes;
  size_t *expr_count;
} beet_ast_expr_pool;

typedef struct beet_parser_allocation {
  void *ptr;
  struct beet_parser_allocation *next;
} beet_parser_allocation;

static beet_parser_allocation *beet_parser_allocations = NULL;
static int beet_parser_cleanup_registered = 0;

static void beet_parser_cleanup_allocations(void) {
  beet_parser_allocation *allocation;

  allocation = beet_parser_allocations;
  while (allocation != NULL) {
    beet_parser_allocation *next = allocation->next;

    free(allocation->ptr);
    free(allocation);
    allocation = next;
  }

  beet_parser_allocations = NULL;
}

static void *beet_parser_alloc_zeroed(size_t size) {
  beet_parser_allocation *allocation;
  void *ptr;

  if (size == 0U) {
    return NULL;
  }

  ptr = calloc(1U, size);
  if (ptr == NULL) {
    return NULL;
  }

  allocation = malloc(sizeof(*allocation));
  if (allocation == NULL) {
    free(ptr);
    return NULL;
  }

  if (!beet_parser_cleanup_registered) {
    atexit(beet_parser_cleanup_allocations);
    beet_parser_cleanup_registered = 1;
  }

  allocation->ptr = ptr;
  allocation->next = beet_parser_allocations;
  beet_parser_allocations = allocation;
  return ptr;
}

static beet_source_span beet_parser_zero_span(void) {
  beet_source_span span;

  memset(&span, 0, sizeof(span));
  return span;
}

static beet_source_span beet_parser_span_from_bounds(beet_source_span start,
                                                     beet_source_span end) {
  beet_source_span span;

  span.start = start.start;
  span.end = end.end;
  return span;
}

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

static int beet_expect_with_token(beet_parser *parser, beet_token_kind kind,
                                  beet_token *out) {
  assert(parser != NULL);
  assert(out != NULL);

  if (parser->current.kind != kind) {
    return 0;
  }

  *out = parser->current;
  beet_parser_advance(parser);
  return 1;
}

void beet_parser_init(beet_parser *parser, const beet_source_file *file) {
  assert(parser != NULL);
  assert(file != NULL);

  memset(parser, 0, sizeof(*parser));
  beet_lexer_init(&parser->lexer, file);
  beet_parser_advance(parser);
}

static int beet_parser_parse_param(beet_parser *parser, beet_ast_param *out) {
  beet_token name_token;
  beet_token type_token;

  assert(parser != NULL);
  assert(out != NULL);

  memset(out, 0, sizeof(*out));

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  name_token = parser->current;
  out->name = beet_intern_slice(name_token.lexeme, name_token.lexeme_len);
  if (out->name == NULL) {
    return 0;
  }
  out->name_len = name_token.lexeme_len;
  beet_parser_advance(parser);

  if (!beet_expect(parser, BEET_TOKEN_KW_IS)) {
    return 0;
  }

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  type_token = parser->current;
  out->type_name = beet_intern_slice(type_token.lexeme, type_token.lexeme_len);
  if (out->type_name == NULL) {
    return 0;
  }
  out->type_name_len = type_token.lexeme_len;
  out->span = beet_parser_span_from_bounds(name_token.span, type_token.span);
  beet_parser_advance(parser);

  return 1;
}

static void beet_ast_expr_init(beet_ast_expr *expr);

static int beet_parser_parse_expr(beet_parser *parser, beet_ast_expr_pool *pool,
                                  beet_ast_expr *out);

static void beet_ast_expr_init(beet_ast_expr *expr) {
  assert(expr != NULL);

  expr->kind = BEET_AST_EXPR_INVALID;
  expr->text = NULL;
  expr->text_len = 0U;
  expr->span = beet_parser_zero_span();
  expr->is_resolved = 0;
  expr->resolved_depth = 0U;
  expr->resolved_is_mutable = 0;
  expr->call_is_resolved = 0;
  expr->call_target_index = 0U;
  expr->resolved_type_decl = NULL;
  expr->resolved_field_decl = NULL;
  expr->resolved_variant_decl = NULL;
  expr->resolved_variant_index = 0U;
  expr->unary_op = BEET_AST_UNARY_NEGATE;
  expr->binary_op = BEET_AST_BINARY_ADD;
  expr->left = NULL;
  expr->right = NULL;
  expr->arg_count = 0U;
  expr->field_init_count = 0U;
}

static void beet_ast_stmt_init(beet_ast_stmt *stmt) {
  assert(stmt != NULL);

  memset(stmt, 0, sizeof(*stmt));
  stmt->kind = BEET_AST_STMT_INVALID;
  stmt->span = beet_parser_zero_span();
}

static beet_ast_expr_pool
beet_ast_expr_pool_from_function(beet_ast_function *function) {
  beet_ast_expr_pool pool;

  assert(function != NULL);

  pool.expr_nodes = function->expr_nodes;
  pool.expr_count = &function->expr_count;
  return pool;
}

static beet_ast_expr_pool
beet_ast_expr_pool_from_parser_binding(beet_parser *parser) {
  beet_ast_expr_pool pool;

  assert(parser != NULL);

  if (parser->binding_expr_nodes == NULL) {
    parser->binding_expr_nodes = beet_parser_alloc_zeroed(
        sizeof(beet_ast_expr) * BEET_AST_MAX_EXPR_NODES);
  }

  pool.expr_nodes = parser->binding_expr_nodes;
  pool.expr_count = &parser->binding_expr_count;
  return pool;
}

static beet_ast_expr *beet_ast_expr_alloc(beet_ast_expr_pool *pool) {
  beet_ast_expr *expr;

  assert(pool != NULL);
  assert(pool->expr_nodes != NULL);
  assert(pool->expr_count != NULL);

  if (*pool->expr_count >= BEET_AST_MAX_EXPR_NODES) {
    return NULL;
  }

  expr = &pool->expr_nodes[*pool->expr_count];
  *pool->expr_count += 1U;
  beet_ast_expr_init(expr);
  return expr;
}

static beet_ast_expr *beet_ast_expr_store(beet_ast_expr_pool *pool,
                                          const beet_ast_expr *source) {
  beet_ast_expr *expr;

  assert(pool != NULL);
  assert(source != NULL);

  expr = beet_ast_expr_alloc(pool);
  if (expr == NULL) {
    return NULL;
  }

  *expr = *source;
  return expr;
}

static beet_ast_stmt *beet_ast_stmt_alloc(beet_ast_function *function) {
  beet_ast_stmt *stmt;

  assert(function != NULL);
  assert(function->stmt_nodes != NULL);

  if (function->stmt_node_count >= BEET_AST_MAX_STMT_NODES) {
    return NULL;
  }

  stmt = &function->stmt_nodes[function->stmt_node_count];
  function->stmt_node_count += 1U;
  beet_ast_stmt_init(stmt);
  return stmt;
}

static int beet_parser_prepare_function_storage(beet_ast_function *function) {
  assert(function != NULL);

  function->body =
      beet_parser_alloc_zeroed(sizeof(beet_ast_stmt) * BEET_AST_MAX_BODY_STMTS);
  function->stmt_nodes =
      beet_parser_alloc_zeroed(sizeof(beet_ast_stmt) * BEET_AST_MAX_STMT_NODES);
  function->expr_nodes =
      beet_parser_alloc_zeroed(sizeof(beet_ast_expr) * BEET_AST_MAX_EXPR_NODES);

  return function->body != NULL && function->stmt_nodes != NULL &&
         function->expr_nodes != NULL;
}

static int beet_parser_parse_primary_expr(beet_parser *parser,
                                          beet_ast_expr_pool *pool,
                                          beet_ast_expr *out);

static int beet_parser_paren_starts_construct(beet_parser *parser) {
  beet_lexer lookahead;
  beet_token first;
  beet_token second;

  assert(parser != NULL);
  assert(parser->current.kind == BEET_TOKEN_LPAREN);

  lookahead = parser->lexer;
  first = beet_lexer_next(&lookahead);
  if (first.kind == BEET_TOKEN_RPAREN) {
    return 0;
  }

  if (first.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  second = beet_lexer_next(&lookahead);
  return second.kind == BEET_TOKEN_EQUAL;
}

static int beet_parser_parse_call_expr(beet_parser *parser,
                                       beet_ast_expr_pool *pool,
                                       const beet_token *callee_token,
                                       beet_ast_expr *out) {
  beet_token close_token;

  assert(parser != NULL);
  assert(pool != NULL);
  assert(callee_token != NULL);
  assert(out != NULL);

  beet_ast_expr_init(out);
  out->kind = BEET_AST_EXPR_CALL;
  out->text = beet_intern_slice(callee_token->lexeme, callee_token->lexeme_len);
  if (out->text == NULL) {
    return 0;
  }
  out->text_len = callee_token->lexeme_len;
  out->span = callee_token->span;

  if (!beet_expect(parser, BEET_TOKEN_LPAREN)) {
    return 0;
  }

  while (parser->current.kind != BEET_TOKEN_RPAREN) {
    beet_ast_expr *arg;

    if (out->arg_count >= BEET_AST_MAX_PARAMS) {
      return 0;
    }

    arg = beet_ast_expr_alloc(pool);
    if (arg == NULL) {
      return 0;
    }

    if (!beet_parser_parse_expr(parser, pool, arg)) {
      return 0;
    }

    out->args[out->arg_count] = arg;
    out->arg_count += 1U;

    if (beet_parser_match(parser, BEET_TOKEN_COMMA)) {
      continue;
    }

    break;
  }

  if (!beet_expect_with_token(parser, BEET_TOKEN_RPAREN, &close_token)) {
    return 0;
  }

  out->span =
      beet_parser_span_from_bounds(callee_token->span, close_token.span);
  return 1;
}

static int beet_parser_parse_choice_construct_expr(
    beet_parser *parser, beet_ast_expr_pool *pool, const char *type_name,
    size_t type_name_len, beet_source_span type_span,
    const beet_token *variant_token, beet_ast_expr *out) {
  beet_ast_expr *value;
  beet_token close_token;

  assert(parser != NULL);
  assert(pool != NULL);
  assert(type_name != NULL);
  assert(variant_token != NULL);
  assert(out != NULL);

  beet_ast_expr_init(out);
  out->kind = BEET_AST_EXPR_CONSTRUCT;
  out->text = type_name;
  out->text_len = type_name_len;
  out->span = beet_parser_span_from_bounds(type_span, variant_token->span);
  out->field_init_count = 1U;
  out->field_inits[0].name =
      beet_intern_slice(variant_token->lexeme, variant_token->lexeme_len);
  if (out->field_inits[0].name == NULL) {
    return 0;
  }
  out->field_inits[0].name_len = variant_token->lexeme_len;
  out->field_inits[0].value = NULL;

  if (!beet_expect(parser, BEET_TOKEN_LPAREN)) {
    return 0;
  }

  if (parser->current.kind == BEET_TOKEN_RPAREN) {
    if (!beet_expect_with_token(parser, BEET_TOKEN_RPAREN, &close_token)) {
      return 0;
    }
    out->span = beet_parser_span_from_bounds(type_span, close_token.span);
    return 1;
  }

  value = beet_ast_expr_alloc(pool);
  if (value == NULL) {
    return 0;
  }

  if (!beet_parser_parse_expr(parser, pool, value)) {
    return 0;
  }

  out->field_inits[0].value = value;
  if (!beet_expect_with_token(parser, BEET_TOKEN_RPAREN, &close_token)) {
    return 0;
  }

  out->span = beet_parser_span_from_bounds(type_span, close_token.span);
  return 1;
}

static int beet_parser_parse_construct_expr(beet_parser *parser,
                                            beet_ast_expr_pool *pool,
                                            const beet_token *type_token,
                                            beet_ast_expr *out) {
  beet_token close_token;

  assert(parser != NULL);
  assert(pool != NULL);
  assert(type_token != NULL);
  assert(out != NULL);

  beet_ast_expr_init(out);
  out->kind = BEET_AST_EXPR_CONSTRUCT;
  out->text = beet_intern_slice(type_token->lexeme, type_token->lexeme_len);
  if (out->text == NULL) {
    return 0;
  }
  out->text_len = type_token->lexeme_len;
  out->span = type_token->span;

  if (!beet_expect(parser, BEET_TOKEN_LPAREN)) {
    return 0;
  }

  while (parser->current.kind != BEET_TOKEN_RPAREN) {
    beet_ast_expr *value;

    if (out->field_init_count >= BEET_AST_MAX_FIELDS) {
      return 0;
    }

    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
      return 0;
    }

    out->field_inits[out->field_init_count].name =
        beet_intern_slice(parser->current.lexeme, parser->current.lexeme_len);
    if (out->field_inits[out->field_init_count].name == NULL) {
      return 0;
    }
    out->field_inits[out->field_init_count].name_len =
        parser->current.lexeme_len;
    beet_parser_advance(parser);

    if (!beet_expect(parser, BEET_TOKEN_EQUAL)) {
      return 0;
    }

    value = beet_ast_expr_alloc(pool);
    if (value == NULL) {
      return 0;
    }

    if (!beet_parser_parse_expr(parser, pool, value)) {
      return 0;
    }

    out->field_inits[out->field_init_count].value = value;
    out->field_init_count += 1U;

    if (beet_parser_match(parser, BEET_TOKEN_COMMA)) {
      continue;
    }

    break;
  }

  if (!beet_expect_with_token(parser, BEET_TOKEN_RPAREN, &close_token)) {
    return 0;
  }

  out->span = beet_parser_span_from_bounds(type_token->span, close_token.span);
  return 1;
}

static int beet_parser_parse_primary_expr(beet_parser *parser,
                                          beet_ast_expr_pool *pool,
                                          beet_ast_expr *out) {
  beet_token identifier_token;

  assert(parser != NULL);
  assert(pool != NULL);
  assert(out != NULL);

  beet_ast_expr_init(out);

  if (parser->current.kind == BEET_TOKEN_INT_LITERAL) {
    out->kind = BEET_AST_EXPR_INT_LITERAL;
    out->text = parser->current.lexeme;
    out->text_len = parser->current.lexeme_len;
    out->span = parser->current.span;
    beet_parser_advance(parser);
    return 1;
  }

  if (parser->current.kind == BEET_TOKEN_IDENTIFIER) {
    int is_bool_name;

    identifier_token = parser->current;
    beet_parser_advance(parser);

    is_bool_name = (identifier_token.lexeme_len == 4U &&
                    strncmp(identifier_token.lexeme, "true", 4U) == 0) ||
                   (identifier_token.lexeme_len == 5U &&
                    strncmp(identifier_token.lexeme, "false", 5U) == 0);

    if (!is_bool_name && parser->current.kind == BEET_TOKEN_LPAREN) {
      if (beet_parser_paren_starts_construct(parser)) {
        return beet_parser_parse_construct_expr(parser, pool, &identifier_token,
                                                out);
      }

      return beet_parser_parse_call_expr(parser, pool, &identifier_token, out);
    }

    out->text =
        beet_intern_slice(identifier_token.lexeme, identifier_token.lexeme_len);
    if (out->text == NULL) {
      return 0;
    }
    out->text_len = identifier_token.lexeme_len;
    out->span = identifier_token.span;

    if ((out->text_len == 4U && strncmp(out->text, "true", 4U) == 0) ||
        (out->text_len == 5U && strncmp(out->text, "false", 5U) == 0)) {
      out->kind = BEET_AST_EXPR_BOOL_LITERAL;
    } else {
      out->kind = BEET_AST_EXPR_NAME;
    }

    return 1;
  }

  if (beet_parser_match(parser, BEET_TOKEN_LPAREN)) {
    if (!beet_parser_parse_expr(parser, pool, out)) {
      return 0;
    }

    return beet_expect(parser, BEET_TOKEN_RPAREN);
  }

  return 0;
}

static int beet_parser_parse_postfix_expr(beet_parser *parser,
                                          beet_ast_expr_pool *pool,
                                          beet_ast_expr *out) {
  assert(parser != NULL);
  assert(pool != NULL);
  assert(out != NULL);

  if (!beet_parser_parse_primary_expr(parser, pool, out)) {
    return 0;
  }

  while (beet_parser_match(parser, BEET_TOKEN_DOT)) {
    beet_ast_expr *base;
    beet_token member_token;
    const char *type_name;
    size_t type_name_len;

    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
      return 0;
    }

    member_token = parser->current;
    beet_parser_advance(parser);

    if (out->kind == BEET_AST_EXPR_NAME &&
        parser->current.kind == BEET_TOKEN_LPAREN) {
      type_name = out->text;
      type_name_len = out->text_len;
      if (!beet_parser_parse_choice_construct_expr(parser, pool, type_name,
                                                   type_name_len, out->span,
                                                   &member_token, out)) {
        return 0;
      }
      continue;
    }

    base = beet_ast_expr_store(pool, out);
    if (base == NULL) {
      return 0;
    }

    beet_ast_expr_init(out);
    out->kind = BEET_AST_EXPR_FIELD;
    out->text = beet_intern_slice(member_token.lexeme, member_token.lexeme_len);
    if (out->text == NULL) {
      return 0;
    }
    out->text_len = member_token.lexeme_len;
    out->left = base;
    out->span = beet_parser_span_from_bounds(base->span, member_token.span);
  }

  return 1;
}

static int beet_parser_parse_unary_expr(beet_parser *parser,
                                        beet_ast_expr_pool *pool,
                                        beet_ast_expr *out) {
  beet_token operator_token;
  beet_ast_expr *operand;

  assert(parser != NULL);
  assert(pool != NULL);
  assert(out != NULL);

  if (parser->current.kind != BEET_TOKEN_MINUS) {
    return beet_parser_parse_postfix_expr(parser, pool, out);
  }

  operator_token = parser->current;
  beet_parser_advance(parser);

  operand = beet_ast_expr_alloc(pool);
  if (operand == NULL) {
    return 0;
  }

  if (!beet_parser_parse_unary_expr(parser, pool, operand)) {
    return 0;
  }

  beet_ast_expr_init(out);
  out->kind = BEET_AST_EXPR_UNARY;
  out->text = operator_token.lexeme;
  out->text_len = operator_token.lexeme_len;
  out->unary_op = BEET_AST_UNARY_NEGATE;
  out->left = operand;
  out->span = beet_parser_span_from_bounds(operator_token.span, operand->span);
  return 1;
}

static int beet_parser_parse_factor_expr(beet_parser *parser,
                                         beet_ast_expr_pool *pool,
                                         beet_ast_expr *out) {
  assert(parser != NULL);
  assert(pool != NULL);
  assert(out != NULL);

  if (!beet_parser_parse_unary_expr(parser, pool, out)) {
    return 0;
  }

  while (parser->current.kind == BEET_TOKEN_STAR ||
         parser->current.kind == BEET_TOKEN_SLASH) {
    beet_token operator_token;
    beet_ast_expr *left;
    beet_ast_expr *right;

    operator_token = parser->current;
    beet_parser_advance(parser);

    left = beet_ast_expr_store(pool, out);
    if (left == NULL) {
      return 0;
    }

    right = beet_ast_expr_alloc(pool);
    if (right == NULL) {
      return 0;
    }

    if (!beet_parser_parse_unary_expr(parser, pool, right)) {
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
    out->span = beet_parser_span_from_bounds(left->span, right->span);
  }

  return 1;
}

static int beet_parser_parse_term_expr(beet_parser *parser,
                                       beet_ast_expr_pool *pool,
                                       beet_ast_expr *out) {
  assert(parser != NULL);
  assert(pool != NULL);
  assert(out != NULL);

  if (!beet_parser_parse_factor_expr(parser, pool, out)) {
    return 0;
  }

  while (parser->current.kind == BEET_TOKEN_PLUS ||
         parser->current.kind == BEET_TOKEN_MINUS) {
    beet_token operator_token;
    beet_ast_expr *left;
    beet_ast_expr *right;

    operator_token = parser->current;
    beet_parser_advance(parser);

    left = beet_ast_expr_store(pool, out);
    if (left == NULL) {
      return 0;
    }

    right = beet_ast_expr_alloc(pool);
    if (right == NULL) {
      return 0;
    }

    if (!beet_parser_parse_factor_expr(parser, pool, right)) {
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
    out->span = beet_parser_span_from_bounds(left->span, right->span);
  }

  return 1;
}

static int beet_parser_parse_expr(beet_parser *parser, beet_ast_expr_pool *pool,
                                  beet_ast_expr *out) {
  assert(parser != NULL);
  assert(pool != NULL);
  assert(out != NULL);

  if (!beet_parser_parse_term_expr(parser, pool, out)) {
    return 0;
  }

  while (parser->current.kind == BEET_TOKEN_EQUAL_EQUAL ||
         parser->current.kind == BEET_TOKEN_BANG_EQUAL ||
         parser->current.kind == BEET_TOKEN_LESS ||
         parser->current.kind == BEET_TOKEN_LESS_EQUAL ||
         parser->current.kind == BEET_TOKEN_GREATER ||
         parser->current.kind == BEET_TOKEN_GREATER_EQUAL) {
    beet_token operator_token;
    beet_ast_expr *left;
    beet_ast_expr *right;

    operator_token = parser->current;
    beet_parser_advance(parser);

    left = beet_ast_expr_store(pool, out);
    if (left == NULL) {
      return 0;
    }

    right = beet_ast_expr_alloc(pool);
    if (right == NULL) {
      return 0;
    }

    if (!beet_parser_parse_term_expr(parser, pool, right)) {
      return 0;
    }

    beet_ast_expr_init(out);
    out->kind = BEET_AST_EXPR_BINARY;
    out->text = operator_token.lexeme;
    out->text_len = operator_token.lexeme_len;
    switch (operator_token.kind) {
    case BEET_TOKEN_EQUAL_EQUAL:
      out->binary_op = BEET_AST_BINARY_EQ;
      break;
    case BEET_TOKEN_BANG_EQUAL:
      out->binary_op = BEET_AST_BINARY_NE;
      break;
    case BEET_TOKEN_LESS:
      out->binary_op = BEET_AST_BINARY_LT;
      break;
    case BEET_TOKEN_LESS_EQUAL:
      out->binary_op = BEET_AST_BINARY_LE;
      break;
    case BEET_TOKEN_GREATER:
      out->binary_op = BEET_AST_BINARY_GT;
      break;
    default:
      out->binary_op = BEET_AST_BINARY_GE;
      break;
    }
    out->left = left;
    out->right = right;
    out->span = beet_parser_span_from_bounds(left->span, right->span);
  }

  return 1;
}

static int beet_parser_parse_binding_with_pool(beet_parser *parser,
                                               beet_ast_expr_pool *pool,
                                               beet_ast_binding *out) {
  int is_mutable = 0;
  beet_token start_token;

  assert(parser != NULL);
  assert(pool != NULL);
  assert(out != NULL);

  if (parser->current.kind == BEET_TOKEN_KW_BIND) {
    is_mutable = 0;
  } else if (parser->current.kind == BEET_TOKEN_KW_MUTABLE) {
    is_mutable = 1;
  } else {
    return 0;
  }

  start_token = parser->current;
  out->name = NULL;
  out->name_len = 0U;
  out->span = beet_parser_zero_span();
  out->is_mutable = 0;
  out->has_type = 0;
  out->type_name = NULL;
  out->type_name_len = 0U;
  out->resolved_type_decl = NULL;
  out->value_text = NULL;
  out->value_len = 0U;
  beet_ast_expr_init(&out->expr);

  beet_parser_advance(parser);

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  out->name =
      beet_intern_slice(parser->current.lexeme, parser->current.lexeme_len);
  if (out->name == NULL) {
    return 0;
  }
  out->name_len = parser->current.lexeme_len;
  out->is_mutable = is_mutable;

  beet_parser_advance(parser);

  if (beet_parser_match(parser, BEET_TOKEN_KW_IS)) {
    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
      return 0;
    }

    out->has_type = 1;
    out->type_name =
        beet_intern_slice(parser->current.lexeme, parser->current.lexeme_len);
    if (out->type_name == NULL) {
      return 0;
    }
    out->type_name_len = parser->current.lexeme_len;

    beet_parser_advance(parser);
  }

  if (!beet_expect(parser, BEET_TOKEN_EQUAL)) {
    return 0;
  }

  if (parser->current.kind == BEET_TOKEN_EOF) {
    return 0;
  }

  if (!beet_parser_parse_expr(parser, pool, &out->expr)) {
    return 0;
  }

  if (out->expr.kind == BEET_AST_EXPR_INT_LITERAL ||
      out->expr.kind == BEET_AST_EXPR_BOOL_LITERAL ||
      out->expr.kind == BEET_AST_EXPR_NAME) {
    out->value_text = out->expr.text;
    out->value_len = out->expr.text_len;
  }

  out->span = beet_parser_span_from_bounds(start_token.span, out->expr.span);

  return 1;
}

int beet_parser_parse_binding(beet_parser *parser, beet_ast_binding *out) {
  beet_ast_expr_pool pool;

  assert(parser != NULL);
  assert(out != NULL);

  parser->binding_expr_count = 0U;
  pool = beet_ast_expr_pool_from_parser_binding(parser);
  return beet_parser_parse_binding_with_pool(parser, &pool, out);
}

static int beet_parser_parse_stmt(beet_parser *parser,
                                  beet_ast_function *function,
                                  beet_ast_stmt *out);

static int beet_parser_parse_assignment_stmt(beet_parser *parser,
                                             beet_ast_function *function,
                                             beet_ast_stmt *out) {
  beet_token name_token;

  assert(parser != NULL);
  assert(function != NULL);
  assert(out != NULL);

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  out->kind = BEET_AST_STMT_ASSIGNMENT;
  name_token = parser->current;
  out->assignment.name =
      beet_intern_slice(name_token.lexeme, name_token.lexeme_len);
  if (out->assignment.name == NULL) {
    return 0;
  }
  out->assignment.name_len = name_token.lexeme_len;
  out->assignment.span = name_token.span;
  beet_parser_advance(parser);

  if (!beet_expect(parser, BEET_TOKEN_EQUAL)) {
    return 0;
  }

  {
    beet_ast_expr_pool pool = beet_ast_expr_pool_from_function(function);
    if (!beet_parser_parse_expr(parser, &pool, &out->expr)) {
      return 0;
    }
  }

  out->span = beet_parser_span_from_bounds(name_token.span, out->expr.span);
  return 1;
}

static int beet_parser_parse_while_stmt(beet_parser *parser,
                                        beet_ast_function *function,
                                        beet_ast_stmt *out) {
  size_t body_start;
  beet_token while_token;
  beet_token close_token;

  assert(parser != NULL);
  assert(function != NULL);
  assert(out != NULL);

  while_token = parser->current;
  if (!beet_expect(parser, BEET_TOKEN_KW_WHILE)) {
    return 0;
  }

  out->kind = BEET_AST_STMT_WHILE;
  {
    beet_ast_expr_pool pool = beet_ast_expr_pool_from_function(function);
    if (!beet_parser_parse_expr(parser, &pool, &out->condition)) {
      return 0;
    }
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

  if (!beet_expect_with_token(parser, BEET_TOKEN_RBRACE, &close_token)) {
    return 0;
  }

  if (out->loop_body_count > 0U) {
    out->loop_body = &function->stmt_nodes[body_start];
  }

  out->span = beet_parser_span_from_bounds(while_token.span, close_token.span);
  return 1;
}

static int beet_parser_parse_if_stmt(beet_parser *parser,
                                     beet_ast_function *function,
                                     beet_ast_stmt *out) {
  size_t body_start;
  size_t else_start;
  beet_token if_token;
  beet_token close_token;

  assert(parser != NULL);
  assert(function != NULL);
  assert(out != NULL);

  if_token = parser->current;
  if (!beet_expect(parser, BEET_TOKEN_KW_IF)) {
    return 0;
  }

  out->kind = BEET_AST_STMT_IF;
  {
    beet_ast_expr_pool pool = beet_ast_expr_pool_from_function(function);
    if (!beet_parser_parse_expr(parser, &pool, &out->condition)) {
      return 0;
    }
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

  if (!beet_expect_with_token(parser, BEET_TOKEN_RBRACE, &close_token)) {
    return 0;
  }

  if (out->then_body_count > 0U) {
    out->then_body = &function->stmt_nodes[body_start];
  }

  if (!beet_parser_match(parser, BEET_TOKEN_KW_ELSE)) {
    out->span = beet_parser_span_from_bounds(if_token.span, close_token.span);
    return 1;
  }

  if (!beet_expect(parser, BEET_TOKEN_LBRACE)) {
    return 0;
  }

  else_start = function->stmt_node_count;
  while (parser->current.kind != BEET_TOKEN_RBRACE) {
    beet_ast_stmt *nested_stmt;

    nested_stmt = beet_ast_stmt_alloc(function);
    if (nested_stmt == NULL) {
      return 0;
    }

    if (!beet_parser_parse_stmt(parser, function, nested_stmt)) {
      return 0;
    }

    out->else_body_count += 1U;
  }

  if (!beet_expect_with_token(parser, BEET_TOKEN_RBRACE, &close_token)) {
    return 0;
  }

  if (out->else_body_count > 0U) {
    out->else_body = &function->stmt_nodes[else_start];
  }

  out->span = beet_parser_span_from_bounds(if_token.span, close_token.span);
  return 1;
}

static int beet_parser_parse_match_stmt(beet_parser *parser,
                                        beet_ast_function *function,
                                        beet_ast_stmt *out) {
  beet_token match_token;
  beet_token close_token;

  assert(parser != NULL);
  assert(function != NULL);
  assert(out != NULL);

  match_token = parser->current;
  if (!beet_expect(parser, BEET_TOKEN_KW_MATCH)) {
    return 0;
  }

  out->kind = BEET_AST_STMT_MATCH;
  {
    beet_ast_expr_pool pool = beet_ast_expr_pool_from_function(function);
    if (!beet_parser_parse_expr(parser, &pool, &out->match_expr)) {
      return 0;
    }
  }

  if (!beet_expect(parser, BEET_TOKEN_LBRACE)) {
    return 0;
  }

  while (parser->current.kind != BEET_TOKEN_RBRACE) {
    beet_ast_match_case *match_case;
    size_t body_start;
    beet_token case_token;
    beet_token case_close_token;

    if (out->match_case_count >= BEET_AST_MAX_VARIANTS) {
      return 0;
    }

    case_token = parser->current;
    if (!beet_expect(parser, BEET_TOKEN_KW_CASE)) {
      return 0;
    }

    if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
      return 0;
    }

    match_case = &out->match_cases[out->match_case_count];
    match_case->variant_name =
        beet_intern_slice(parser->current.lexeme, parser->current.lexeme_len);
    if (match_case->variant_name == NULL) {
      return 0;
    }
    match_case->variant_name_len = parser->current.lexeme_len;
    beet_parser_advance(parser);

    if (beet_parser_match(parser, BEET_TOKEN_LPAREN)) {
      match_case->binds_payload = 1;
      if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
        return 0;
      }
      match_case->binding_name =
          beet_intern_slice(parser->current.lexeme, parser->current.lexeme_len);
      if (match_case->binding_name == NULL) {
        return 0;
      }
      match_case->binding_name_len = parser->current.lexeme_len;
      beet_parser_advance(parser);
      if (!beet_expect(parser, BEET_TOKEN_RPAREN)) {
        return 0;
      }
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

      match_case->body_count += 1U;
    }

    if (!beet_expect_with_token(parser, BEET_TOKEN_RBRACE, &case_close_token)) {
      return 0;
    }

    if (match_case->body_count > 0U) {
      match_case->body = &function->stmt_nodes[body_start];
    }
    match_case->span =
        beet_parser_span_from_bounds(case_token.span, case_close_token.span);

    out->match_case_count += 1U;
  }

  if (!beet_expect_with_token(parser, BEET_TOKEN_RBRACE, &close_token)) {
    return 0;
  }

  out->span = beet_parser_span_from_bounds(match_token.span, close_token.span);
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
    beet_ast_expr_pool pool = beet_ast_expr_pool_from_function(function);

    out->kind = BEET_AST_STMT_BINDING;
    if (!beet_parser_parse_binding_with_pool(parser, &pool, &out->binding)) {
      return 0;
    }
    out->span = out->binding.span;
    return 1;
  }

  if (parser->current.kind == BEET_TOKEN_IDENTIFIER) {
    return beet_parser_parse_assignment_stmt(parser, function, out);
  }

  if (parser->current.kind == BEET_TOKEN_KW_RETURN) {
    beet_ast_expr_pool pool = beet_ast_expr_pool_from_function(function);
    beet_token return_token;

    out->kind = BEET_AST_STMT_RETURN;
    return_token = parser->current;
    beet_parser_advance(parser);
    if (!beet_parser_parse_expr(parser, &pool, &out->expr)) {
      return 0;
    }
    out->span = beet_parser_span_from_bounds(return_token.span, out->expr.span);
    return 1;
  }

  if (parser->current.kind == BEET_TOKEN_KW_IF) {
    return beet_parser_parse_if_stmt(parser, function, out);
  }

  if (parser->current.kind == BEET_TOKEN_KW_WHILE) {
    return beet_parser_parse_while_stmt(parser, function, out);
  }

  if (parser->current.kind == BEET_TOKEN_KW_MATCH) {
    return beet_parser_parse_match_stmt(parser, function, out);
  }

  out->kind = BEET_AST_STMT_INVALID;
  return 0;
}

int beet_parser_parse_function(beet_parser *parser, beet_ast_function *out) {
  beet_token function_token;
  beet_token close_token;

  assert(parser != NULL);
  assert(out != NULL);

  memset(out, 0, sizeof(*out));
  if (!beet_parser_prepare_function_storage(out)) {
    return 0;
  }
  function_token = parser->current;
  if (!beet_expect(parser, BEET_TOKEN_KW_FUNCTION)) {
    return 0;
  }

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  out->name =
      beet_intern_slice(parser->current.lexeme, parser->current.lexeme_len);
  if (out->name == NULL) {
    return 0;
  }
  out->name_len = parser->current.lexeme_len;
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

  out->return_type_name =
      beet_intern_slice(parser->current.lexeme, parser->current.lexeme_len);
  if (out->return_type_name == NULL) {
    return 0;
  }
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

  if (!beet_expect_with_token(parser, BEET_TOKEN_RBRACE, &close_token)) {
    return 0;
  }

  out->span =
      beet_parser_span_from_bounds(function_token.span, close_token.span);
  return 1;
}

static int beet_parser_parse_field(beet_parser *parser, beet_ast_field *out) {
  beet_token name_token;
  beet_token type_token;

  assert(parser != NULL);
  assert(out != NULL);

  memset(out, 0, sizeof(*out));

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  name_token = parser->current;
  out->name = beet_intern_slice(name_token.lexeme, name_token.lexeme_len);
  if (out->name == NULL) {
    return 0;
  }
  out->name_len = name_token.lexeme_len;
  beet_parser_advance(parser);

  if (!beet_expect(parser, BEET_TOKEN_KW_IS)) {
    return 0;
  }

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  type_token = parser->current;
  out->type_name = beet_intern_slice(type_token.lexeme, type_token.lexeme_len);
  if (out->type_name == NULL) {
    return 0;
  }
  out->type_name_len = type_token.lexeme_len;
  out->span = beet_parser_span_from_bounds(name_token.span, type_token.span);
  beet_parser_advance(parser);

  return 1;
}

static int beet_parser_parse_choice_variant(beet_parser *parser,
                                            beet_ast_choice_variant *out) {
  beet_token name_token;
  beet_token payload_token;
  beet_token close_token;

  assert(parser != NULL);
  assert(out != NULL);

  memset(out, 0, sizeof(*out));

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  name_token = parser->current;
  out->name = beet_intern_slice(name_token.lexeme, name_token.lexeme_len);
  if (out->name == NULL) {
    return 0;
  }
  out->name_len = name_token.lexeme_len;
  out->span = name_token.span;
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

  payload_token = parser->current;
  out->has_payload = 1;
  out->payload_type_name =
      beet_intern_slice(payload_token.lexeme, payload_token.lexeme_len);
  if (out->payload_type_name == NULL) {
    return 0;
  }
  out->payload_type_name_len = payload_token.lexeme_len;
  beet_parser_advance(parser);

  if (!beet_expect_with_token(parser, BEET_TOKEN_RPAREN, &close_token)) {
    return 0;
  }

  out->span = beet_parser_span_from_bounds(name_token.span, close_token.span);
  return 1;
}

int beet_parser_parse_type_decl(beet_parser *parser, beet_ast_type_decl *out) {
  beet_token type_token;
  beet_token close_token;

  assert(parser != NULL);
  assert(out != NULL);

  memset(out, 0, sizeof(*out));
  type_token = parser->current;
  if (!beet_expect(parser, BEET_TOKEN_KW_TYPE)) {
    return 0;
  }

  if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
    return 0;
  }

  out->name =
      beet_intern_slice(parser->current.lexeme, parser->current.lexeme_len);
  if (out->name == NULL) {
    return 0;
  }
  out->name_len = parser->current.lexeme_len;
  beet_parser_advance(parser);

  if (beet_parser_match(parser, BEET_TOKEN_LPAREN)) {
    while (parser->current.kind != BEET_TOKEN_RPAREN) {
      if (parser->current.kind != BEET_TOKEN_IDENTIFIER) {
        return 0;
      }

      if (out->param_count >= BEET_AST_MAX_PARAMS) {
        return 0;
      }

      out->params[out->param_count].name =
          beet_intern_slice(parser->current.lexeme, parser->current.lexeme_len);
      if (out->params[out->param_count].name == NULL) {
        return 0;
      }
      out->params[out->param_count].name_len = parser->current.lexeme_len;
      out->params[out->param_count].span = parser->current.span;
      out->param_count += 1U;
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

    if (!beet_expect_with_token(parser, BEET_TOKEN_RBRACE, &close_token)) {
      return 0;
    }

    out->span = beet_parser_span_from_bounds(type_token.span, close_token.span);
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

  if (!beet_expect_with_token(parser, BEET_TOKEN_RBRACE, &close_token)) {
    return 0;
  }

  out->span = beet_parser_span_from_bounds(type_token.span, close_token.span);
  return 1;
}
