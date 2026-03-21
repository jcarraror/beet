#include "beet/resolve/scope.h"

#include <assert.h>
#include <string.h>

static int beet_name_equals_slice(const char *left, size_t left_len,
                                  const char *right, size_t right_len) {
  return left_len == right_len && strncmp(left, right, left_len) == 0;
}

void beet_scope_stack_init(beet_scope_stack *stack) {
  assert(stack != NULL);

  stack->symbol_count = 0U;
  stack->scope_depth = 0U;
  stack->scope_starts[0] = 0U;
}

int beet_scope_enter(beet_scope_stack *stack) {
  assert(stack != NULL);

  if (stack->scope_depth + 1U >= BEET_SCOPE_MAX_STACK) {
    return 0;
  }

  stack->scope_depth += 1U;
  stack->scope_starts[stack->scope_depth] = stack->symbol_count;
  return 1;
}

int beet_scope_leave(beet_scope_stack *stack) {
  assert(stack != NULL);

  if (stack->scope_depth == 0U) {
    return 0;
  }

  stack->symbol_count = stack->scope_starts[stack->scope_depth];
  stack->scope_depth -= 1U;
  return 1;
}

int beet_scope_bind(beet_scope_stack *stack, const char *name, int is_mutable) {
  return beet_scope_bind_slice(stack, name, strlen(name), is_mutable);
}

int beet_scope_bind_slice(beet_scope_stack *stack, const char *name,
                          size_t name_len, int is_mutable) {
  size_t start;
  size_t i;

  assert(stack != NULL);
  assert(name != NULL);

  if (stack->symbol_count >= BEET_SCOPE_MAX_SYMBOLS) {
    return 0;
  }

  start = stack->scope_starts[stack->scope_depth];
  for (i = start; i < stack->symbol_count; ++i) {
    if (beet_name_equals_slice(stack->symbols[i].name,
                               stack->symbols[i].name_len, name, name_len)) {
      return 0;
    }
  }

  stack->symbols[stack->symbol_count].name = name;
  stack->symbols[stack->symbol_count].name_len = name_len;
  stack->symbols[stack->symbol_count].depth = stack->scope_depth;
  stack->symbols[stack->symbol_count].is_mutable = is_mutable;
  stack->symbol_count += 1U;
  return 1;
}

const beet_symbol *beet_scope_lookup(const beet_scope_stack *stack,
                                     const char *name) {
  return beet_scope_lookup_slice(stack, name, strlen(name));
}

const beet_symbol *beet_scope_lookup_slice(const beet_scope_stack *stack,
                                           const char *name, size_t name_len) {
  size_t i;

  assert(stack != NULL);
  assert(name != NULL);

  i = stack->symbol_count;
  while (i > 0U) {
    i -= 1U;
    if (beet_name_equals_slice(stack->symbols[i].name,
                               stack->symbols[i].name_len, name, name_len)) {
      return &stack->symbols[i];
    }
  }

  return NULL;
}

static int beet_resolve_assignment(beet_scope_stack *stack,
                                   beet_ast_assignment *assignment) {
  const beet_symbol *symbol;

  assert(stack != NULL);
  assert(assignment != NULL);

  symbol =
      beet_scope_lookup_slice(stack, assignment->name, assignment->name_len);
  if (symbol == NULL) {
    return 0;
  }

  assignment->is_resolved = 1;
  assignment->resolved_depth = symbol->depth;
  assignment->resolved_is_mutable = symbol->is_mutable;
  return 1;
}

static int beet_resolve_expr(beet_scope_stack *stack, beet_ast_expr *expr) {
  const beet_symbol *symbol;

  assert(stack != NULL);
  assert(expr != NULL);

  switch (expr->kind) {
  case BEET_AST_EXPR_INT_LITERAL:
  case BEET_AST_EXPR_BOOL_LITERAL:
    return 1;

  case BEET_AST_EXPR_NAME:
    symbol = beet_scope_lookup_slice(stack, expr->text, expr->text_len);
    if (symbol == NULL) {
      return 0;
    }

    expr->is_resolved = 1;
    expr->resolved_depth = symbol->depth;
    expr->resolved_is_mutable = symbol->is_mutable;
    return 1;

  case BEET_AST_EXPR_CONSTRUCT: {
    size_t i;

    for (i = 0U; i < expr->field_init_count; ++i) {
      if (expr->field_inits[i].value == NULL) {
        return 0;
      }
      if (!beet_resolve_expr(stack, expr->field_inits[i].value)) {
        return 0;
      }
    }
    return 1;
  }

  case BEET_AST_EXPR_FIELD:
    if (expr->left == NULL) {
      return 0;
    }
    return beet_resolve_expr(stack, expr->left);

  case BEET_AST_EXPR_UNARY:
    if (expr->left == NULL) {
      return 0;
    }
    return beet_resolve_expr(stack, expr->left);

  case BEET_AST_EXPR_BINARY:
    if (expr->left == NULL || expr->right == NULL) {
      return 0;
    }
    return beet_resolve_expr(stack, expr->left) &&
           beet_resolve_expr(stack, expr->right);

  default:
    return 0;
  }
}

static int beet_resolve_stmt_list(beet_scope_stack *stack, beet_ast_stmt *stmts,
                                  size_t stmt_count) {
  size_t i;

  assert(stack != NULL);
  assert(stmts != NULL || stmt_count == 0U);

  for (i = 0U; i < stmt_count; ++i) {
    beet_ast_stmt *stmt = &stmts[i];

    switch (stmt->kind) {
    case BEET_AST_STMT_BINDING:
      if (!beet_resolve_expr(stack, &stmt->binding.expr)) {
        return 0;
      }
      if (!beet_scope_bind_slice(stack, stmt->binding.name,
                                 stmt->binding.name_len,
                                 stmt->binding.is_mutable)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_ASSIGNMENT:
      if (!beet_resolve_assignment(stack, &stmt->assignment)) {
        return 0;
      }
      if (!beet_resolve_expr(stack, &stmt->expr)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_RETURN:
      if (!beet_resolve_expr(stack, &stmt->expr)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_IF:
      if (!beet_resolve_expr(stack, &stmt->condition)) {
        return 0;
      }
      if (!beet_scope_enter(stack)) {
        return 0;
      }
      if (!beet_resolve_stmt_list(stack, stmt->then_body,
                                  stmt->then_body_count)) {
        (void)beet_scope_leave(stack);
        return 0;
      }
      if (!beet_scope_leave(stack)) {
        return 0;
      }
      if (stmt->else_body_count > 0U) {
        if (!beet_scope_enter(stack)) {
          return 0;
        }
        if (!beet_resolve_stmt_list(stack, stmt->else_body,
                                    stmt->else_body_count)) {
          (void)beet_scope_leave(stack);
          return 0;
        }
        if (!beet_scope_leave(stack)) {
          return 0;
        }
      }
      break;

    case BEET_AST_STMT_WHILE:
      if (!beet_resolve_expr(stack, &stmt->condition)) {
        return 0;
      }
      if (!beet_scope_enter(stack)) {
        return 0;
      }
      if (!beet_resolve_stmt_list(stack, stmt->loop_body,
                                  stmt->loop_body_count)) {
        (void)beet_scope_leave(stack);
        return 0;
      }
      if (!beet_scope_leave(stack)) {
        return 0;
      }
      break;

    default:
      return 0;
    }
  }

  return 1;
}

int beet_resolve_function(beet_ast_function *function_ast) {
  beet_scope_stack stack;
  size_t i;

  assert(function_ast != NULL);

  beet_scope_stack_init(&stack);

  for (i = 0U; i < function_ast->param_count; ++i) {
    const beet_ast_param *param = &function_ast->params[i];

    if (!beet_scope_bind_slice(&stack, param->name, param->name_len, 0)) {
      return 0;
    }
  }

  return beet_resolve_stmt_list(&stack, function_ast->body,
                                function_ast->body_count);
}
