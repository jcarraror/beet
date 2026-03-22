#include "beet/resolve/scope.h"

#include <assert.h>
#include <string.h>

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
  beet_symbol_id id;
  size_t start;
  size_t i;

  assert(stack != NULL);
  assert(name != NULL);

  id = beet_intern_slice(name, name_len);
  if (id == NULL) {
    return 0;
  }

  if (stack->symbol_count >= BEET_SCOPE_MAX_SYMBOLS) {
    return 0;
  }

  start = stack->scope_starts[stack->scope_depth];
  for (i = start; i < stack->symbol_count; ++i) {
    if (beet_symbol_eq(stack->symbols[i].id, id)) {
      return 0;
    }
  }

  stack->symbols[stack->symbol_count].id = id;
  stack->symbols[stack->symbol_count].name = id;
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
  beet_symbol_id id;
  size_t i;

  assert(stack != NULL);
  assert(name != NULL);

  id = beet_intern_slice(name, name_len);
  if (id == NULL) {
    return NULL;
  }

  i = stack->symbol_count;
  while (i > 0U) {
    i -= 1U;
    if (beet_symbol_eq(stack->symbols[i].id, id)) {
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

static int beet_resolve_expr(beet_scope_stack *stack, beet_ast_expr *expr,
                             const beet_decl_registry *registry) {
  const beet_symbol *symbol;

  assert(stack != NULL);
  assert(expr != NULL);
  assert(registry != NULL);

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

  case BEET_AST_EXPR_CALL: {
    size_t i;
    size_t target_index;

    if (!beet_decl_registry_find_function_index(
            registry, expr->text, expr->text_len, &target_index)) {
      return 0;
    }

    expr->call_is_resolved = 1;
    expr->call_target_index = target_index;

    for (i = 0U; i < expr->arg_count; ++i) {
      if (expr->args[i] == NULL) {
        return 0;
      }
      if (!beet_resolve_expr(stack, expr->args[i], registry)) {
        return 0;
      }
    }
    return 1;
  }

  case BEET_AST_EXPR_CONSTRUCT: {
    size_t i;

    for (i = 0U; i < expr->field_init_count; ++i) {
      if (expr->field_inits[i].value != NULL &&
          !beet_resolve_expr(stack, expr->field_inits[i].value, registry)) {
        return 0;
      }
    }
    return 1;
  }

  case BEET_AST_EXPR_FIELD:
    if (expr->left == NULL) {
      return 0;
    }
    return beet_resolve_expr(stack, expr->left, registry);

  case BEET_AST_EXPR_UNARY:
    if (expr->left == NULL) {
      return 0;
    }
    return beet_resolve_expr(stack, expr->left, registry);

  case BEET_AST_EXPR_BINARY:
    if (expr->left == NULL || expr->right == NULL) {
      return 0;
    }
    return beet_resolve_expr(stack, expr->left, registry) &&
           beet_resolve_expr(stack, expr->right, registry);

  default:
    return 0;
  }
}

static int beet_resolve_stmt_list(beet_scope_stack *stack, beet_ast_stmt *stmts,
                                  size_t stmt_count,
                                  const beet_decl_registry *registry) {
  size_t i;

  assert(stack != NULL);
  assert(stmts != NULL || stmt_count == 0U);
  assert(registry != NULL);

  for (i = 0U; i < stmt_count; ++i) {
    beet_ast_stmt *stmt = &stmts[i];

    switch (stmt->kind) {
    case BEET_AST_STMT_BINDING:
      if (!beet_resolve_expr(stack, &stmt->binding.expr, registry)) {
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
      if (!beet_resolve_expr(stack, &stmt->expr, registry)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_RETURN:
      if (!beet_resolve_expr(stack, &stmt->expr, registry)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_IF:
      if (!beet_resolve_expr(stack, &stmt->condition, registry)) {
        return 0;
      }
      if (!beet_scope_enter(stack)) {
        return 0;
      }
      if (!beet_resolve_stmt_list(stack, stmt->then_body, stmt->then_body_count,
                                  registry)) {
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
                                    stmt->else_body_count, registry)) {
          (void)beet_scope_leave(stack);
          return 0;
        }
        if (!beet_scope_leave(stack)) {
          return 0;
        }
      }
      break;

    case BEET_AST_STMT_WHILE:
      if (!beet_resolve_expr(stack, &stmt->condition, registry)) {
        return 0;
      }
      if (!beet_scope_enter(stack)) {
        return 0;
      }
      if (!beet_resolve_stmt_list(stack, stmt->loop_body, stmt->loop_body_count,
                                  registry)) {
        (void)beet_scope_leave(stack);
        return 0;
      }
      if (!beet_scope_leave(stack)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_MATCH: {
      size_t case_index;

      if (!beet_resolve_expr(stack, &stmt->match_expr, registry)) {
        return 0;
      }

      for (case_index = 0U; case_index < stmt->match_case_count; ++case_index) {
        beet_ast_match_case *match_case = &stmt->match_cases[case_index];

        if (!beet_scope_enter(stack)) {
          return 0;
        }
        if (match_case->binds_payload &&
            !beet_scope_bind_slice(stack, match_case->binding_name,
                                   match_case->binding_name_len, 0)) {
          (void)beet_scope_leave(stack);
          return 0;
        }
        if (!beet_resolve_stmt_list(stack, match_case->body,
                                    match_case->body_count, registry)) {
          (void)beet_scope_leave(stack);
          return 0;
        }
        if (!beet_scope_leave(stack)) {
          return 0;
        }
      }
      break;
    }

    default:
      return 0;
    }
  }

  return 1;
}

int beet_resolve_function_with_registry(beet_ast_function *function_ast,
                                        const beet_decl_registry *registry) {
  beet_scope_stack stack;
  size_t i;

  assert(function_ast != NULL);
  assert(registry != NULL);

  beet_scope_stack_init(&stack);

  for (i = 0U; i < function_ast->param_count; ++i) {
    if (!beet_scope_bind_slice(&stack, function_ast->params[i].name,
                               function_ast->params[i].name_len, 0)) {
      return 0;
    }
  }

  return beet_resolve_stmt_list(&stack, function_ast->body,
                                function_ast->body_count, registry);
}

int beet_resolve_function_with_decls(beet_ast_function *function_ast,
                                     const beet_ast_function *function_decls,
                                     size_t function_count) {
  beet_decl_registry registry;

  beet_decl_registry_init(&registry, NULL, 0U, function_decls, function_count);
  return beet_resolve_function_with_registry(function_ast, &registry);
}

int beet_resolve_function(beet_ast_function *function_ast) {
  return beet_resolve_function_with_decls(function_ast, function_ast, 1U);
}
