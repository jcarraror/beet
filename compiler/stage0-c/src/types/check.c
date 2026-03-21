#include "beet/types/check.h"

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>

typedef struct beet_local_type {
  const char *name;
  size_t name_len;
  beet_type type;
  int is_mutable;
} beet_local_type;

#define BEET_TYPE_CHECK_MAX_LOCALS 128

static beet_type beet_type_from_value_text(const char *text, size_t len) {
  beet_type type;
  size_t i;
  int saw_dot = 0;

  assert(text != NULL);

  type.kind = BEET_TYPE_INVALID;
  type.name = NULL;

  if (len == 4U && strncmp(text, "true", 4U) == 0) {
    type.kind = BEET_TYPE_BOOL;
    type.name = "Bool";
    return type;
  }

  if (len == 5U && strncmp(text, "false", 5U) == 0) {
    type.kind = BEET_TYPE_BOOL;
    type.name = "Bool";
    return type;
  }

  if (len == 2U && strncmp(text, "()", 2U) == 0) {
    type.kind = BEET_TYPE_UNIT;
    type.name = "Unit";
    return type;
  }

  if (len == 0U) {
    return type;
  }

  for (i = 0U; i < len; ++i) {
    if (text[i] == '.') {
      if (saw_dot) {
        return type;
      }
      saw_dot = 1;
      continue;
    }

    if (!isdigit((unsigned char)text[i])) {
      return type;
    }
  }

  if (saw_dot) {
    type.kind = BEET_TYPE_FLOAT;
    type.name = "Float";
  } else {
    type.kind = BEET_TYPE_INT;
    type.name = "Int";
  }

  return type;
}

beet_type beet_type_check_binding(const beet_ast_binding *binding) {
  assert(binding != NULL);

  return beet_type_from_value_text(binding->value_text, binding->value_len);
}

static int beet_name_equals_slice(const char *left, size_t left_len,
                                  const char *right, size_t right_len) {
  return left_len == right_len && strncmp(left, right, left_len) == 0;
}

static const beet_local_type *
beet_find_local_type(const beet_local_type *locals, size_t local_count,
                     const char *name, size_t name_len) {
  size_t i;

  assert(locals != NULL || local_count == 0U);
  assert(name != NULL);

  i = local_count;
  while (i > 0U) {
    i -= 1U;
    if (beet_name_equals_slice(locals[i].name, locals[i].name_len, name,
                               name_len)) {
      return &locals[i];
    }
  }

  return NULL;
}

static beet_type beet_type_check_expr(const beet_ast_expr *expr,
                                      const beet_local_type *locals,
                                      size_t local_count) {
  beet_type type;
  beet_type operand_type;
  beet_type left_type;
  beet_type right_type;
  const beet_local_type *local;

  assert(expr != NULL);

  type.kind = BEET_TYPE_INVALID;
  type.name = NULL;

  switch (expr->kind) {
  case BEET_AST_EXPR_INT_LITERAL:
    type.kind = BEET_TYPE_INT;
    type.name = "Int";
    return type;

  case BEET_AST_EXPR_BOOL_LITERAL:
    type.kind = BEET_TYPE_BOOL;
    type.name = "Bool";
    return type;

  case BEET_AST_EXPR_NAME:
    local =
        beet_find_local_type(locals, local_count, expr->text, expr->text_len);
    if (local == NULL) {
      return type;
    }
    return local->type;

  case BEET_AST_EXPR_UNARY:
    if (expr->unary_op != BEET_AST_UNARY_NEGATE || expr->left == NULL) {
      return type;
    }

    operand_type = beet_type_check_expr(expr->left, locals, local_count);
    if (operand_type.kind != BEET_TYPE_INT) {
      return type;
    }
    return operand_type;

  case BEET_AST_EXPR_BINARY:
    if (expr->left == NULL || expr->right == NULL) {
      return type;
    }

    left_type = beet_type_check_expr(expr->left, locals, local_count);
    right_type = beet_type_check_expr(expr->right, locals, local_count);
    if (left_type.kind != BEET_TYPE_INT || right_type.kind != BEET_TYPE_INT) {
      return type;
    }

    switch (expr->binary_op) {
    case BEET_AST_BINARY_ADD:
    case BEET_AST_BINARY_SUB:
    case BEET_AST_BINARY_MUL:
    case BEET_AST_BINARY_DIV:
      type.kind = BEET_TYPE_INT;
      type.name = "Int";
      return type;

    case BEET_AST_BINARY_EQ:
    case BEET_AST_BINARY_NE:
    case BEET_AST_BINARY_LT:
    case BEET_AST_BINARY_LE:
    case BEET_AST_BINARY_GT:
    case BEET_AST_BINARY_GE:
      type.kind = BEET_TYPE_BOOL;
      type.name = "Bool";
      return type;

    default:
      return type;
    }

  default:
    return type;
  }
}

static int beet_type_check_stmt_list(const beet_ast_stmt *stmts,
                                     size_t stmt_count, beet_type return_type,
                                     beet_local_type *locals,
                                     size_t *local_count) {
  size_t i;

  assert(stmts != NULL || stmt_count == 0U);
  assert(locals != NULL);
  assert(local_count != NULL);

  for (i = 0U; i < stmt_count; ++i) {
    const beet_ast_stmt *stmt = &stmts[i];

    switch (stmt->kind) {
    case BEET_AST_STMT_BINDING: {
      beet_type binding_type;
      beet_type_check_result result;

      if (stmt->binding.has_type) {
        result = beet_type_check_binding_annotation(&stmt->binding);
        if (!result.ok) {
          return 0;
        }
        binding_type = result.declared_type;
      } else {
        binding_type = beet_type_check_binding(&stmt->binding);
        if (!beet_type_is_known(&binding_type)) {
          return 0;
        }
      }

      if (*local_count >= BEET_TYPE_CHECK_MAX_LOCALS) {
        return 0;
      }

      locals[*local_count].name = stmt->binding.name;
      locals[*local_count].name_len = stmt->binding.name_len;
      locals[*local_count].type = binding_type;
      locals[*local_count].is_mutable = stmt->binding.is_mutable;
      *local_count += 1U;
      break;
    }

    case BEET_AST_STMT_ASSIGNMENT: {
      const beet_local_type *target;
      beet_type expr_type;

      target = beet_find_local_type(locals, *local_count, stmt->assignment.name,
                                    stmt->assignment.name_len);
      if (target == NULL || !target->is_mutable) {
        return 0;
      }

      expr_type = beet_type_check_expr(&stmt->expr, locals, *local_count);
      if (expr_type.kind != target->type.kind) {
        return 0;
      }
      break;
    }

    case BEET_AST_STMT_RETURN: {
      beet_type expr_type =
          beet_type_check_expr(&stmt->expr, locals, *local_count);
      if (expr_type.kind != return_type.kind) {
        return 0;
      }
      break;
    }

    case BEET_AST_STMT_IF: {
      beet_type condition_type;
      beet_local_type branch_locals[BEET_TYPE_CHECK_MAX_LOCALS];
      beet_local_type else_locals[BEET_TYPE_CHECK_MAX_LOCALS];
      size_t branch_local_count;
      size_t else_local_count;

      condition_type =
          beet_type_check_expr(&stmt->condition, locals, *local_count);
      if (condition_type.kind != BEET_TYPE_BOOL) {
        return 0;
      }

      memcpy(branch_locals, locals, sizeof(beet_local_type) * (*local_count));
      branch_local_count = *local_count;
      if (!beet_type_check_stmt_list(stmt->then_body, stmt->then_body_count,
                                     return_type, branch_locals,
                                     &branch_local_count)) {
        return 0;
      }

      memcpy(else_locals, locals, sizeof(beet_local_type) * (*local_count));
      else_local_count = *local_count;
      if (!beet_type_check_stmt_list(stmt->else_body, stmt->else_body_count,
                                     return_type, else_locals,
                                     &else_local_count)) {
        return 0;
      }
      break;
    }

    case BEET_AST_STMT_WHILE: {
      beet_type condition_type;
      beet_local_type loop_locals[BEET_TYPE_CHECK_MAX_LOCALS];
      size_t loop_local_count;

      condition_type =
          beet_type_check_expr(&stmt->condition, locals, *local_count);
      if (condition_type.kind != BEET_TYPE_BOOL) {
        return 0;
      }

      memcpy(loop_locals, locals, sizeof(beet_local_type) * (*local_count));
      loop_local_count = *local_count;
      if (!beet_type_check_stmt_list(stmt->loop_body, stmt->loop_body_count,
                                     return_type, loop_locals,
                                     &loop_local_count)) {
        return 0;
      }
      break;
    }

    default:
      return 0;
    }
  }

  return 1;
}

beet_type_check_result
beet_type_check_binding_annotation(const beet_ast_binding *binding) {
  beet_type_check_result result;

  assert(binding != NULL);

  result.ok = 1;
  result.declared_type.kind = BEET_TYPE_INVALID;
  result.declared_type.name = NULL;
  result.value_type = beet_type_check_binding(binding);

  if (!binding->has_type) {
    return result;
  }

  result.declared_type =
      beet_type_from_name_slice(binding->type_name, binding->type_name_len);

  if (!beet_type_is_known(&result.value_type)) {
    result.ok = 0;
    return result;
  }

  if (result.declared_type.kind != result.value_type.kind) {
    result.ok = 0;
  }

  return result;
}

int beet_type_check_function_signature(const beet_ast_function *function_ast) {
  size_t i;
  beet_type return_type;

  assert(function_ast != NULL);

  return_type = beet_type_from_name_slice(function_ast->return_type_name,
                                          function_ast->return_type_name_len);
  if (!beet_type_is_known(&return_type)) {
    return 0;
  }

  for (i = 0U; i < function_ast->param_count; ++i) {
    beet_type param_type =
        beet_type_from_name_slice(function_ast->params[i].type_name,
                                  function_ast->params[i].type_name_len);
    if (!beet_type_is_known(&param_type)) {
      return 0;
    }
  }

  return 1;
}

int beet_type_check_function_body(const beet_ast_function *function_ast) {
  beet_local_type locals[BEET_TYPE_CHECK_MAX_LOCALS];
  beet_type return_type;
  size_t local_count;
  size_t i;

  assert(function_ast != NULL);

  return_type = beet_type_from_name_slice(function_ast->return_type_name,
                                          function_ast->return_type_name_len);
  if (!beet_type_is_known(&return_type)) {
    return 0;
  }

  local_count = 0U;
  for (i = 0U; i < function_ast->param_count; ++i) {
    beet_type param_type =
        beet_type_from_name_slice(function_ast->params[i].type_name,
                                  function_ast->params[i].type_name_len);
    if (!beet_type_is_known(&param_type) ||
        local_count >= BEET_TYPE_CHECK_MAX_LOCALS) {
      return 0;
    }

    locals[local_count].name = function_ast->params[i].name;
    locals[local_count].name_len = function_ast->params[i].name_len;
    locals[local_count].type = param_type;
    locals[local_count].is_mutable = 0;
    local_count += 1U;
  }

  return beet_type_check_stmt_list(function_ast->body, function_ast->body_count,
                                   return_type, locals, &local_count);
}

static int beet_type_name_is_builtin(const char *name, size_t name_len) {
  beet_type type;

  type = beet_type_from_name_slice(name, name_len);
  return beet_type_is_known(&type);
}

static int beet_type_decl_has_param(const beet_ast_type_decl *type_decl,
                                    const char *name, size_t name_len) {
  size_t i;

  assert(type_decl != NULL);
  assert(name != NULL);

  for (i = 0U; i < type_decl->param_count; ++i) {
    if (beet_name_equals_slice(type_decl->params[i].name,
                               type_decl->params[i].name_len, name,
                               name_len)) {
      return 1;
    }
  }

  return 0;
}

static int beet_type_decl_list_has_name(const beet_ast_type_decl *type_decls,
                                        size_t decl_count, const char *name,
                                        size_t name_len) {
  size_t i;

  assert(type_decls != NULL || decl_count == 0U);
  assert(name != NULL);

  for (i = 0U; i < decl_count; ++i) {
    if (beet_name_equals_slice(type_decls[i].name, type_decls[i].name_len,
                               name, name_len)) {
      return 1;
    }
  }

  return 0;
}

static int beet_type_name_is_known_for_decl(const beet_ast_type_decl *type_decl,
                                            const beet_ast_type_decl *type_decls,
                                            size_t decl_count,
                                            const char *name,
                                            size_t name_len) {
  assert(type_decl != NULL);
  assert(name != NULL);

  return beet_type_name_is_builtin(name, name_len) ||
         beet_type_decl_has_param(type_decl, name, name_len) ||
         beet_type_decl_list_has_name(type_decls, decl_count, name, name_len);
}

static int beet_type_decl_names_are_unique(const beet_ast_type_decl *type_decls,
                                           size_t decl_count) {
  size_t i;
  size_t j;

  assert(type_decls != NULL || decl_count == 0U);

  for (i = 0U; i < decl_count; ++i) {
    for (j = i + 1U; j < decl_count; ++j) {
      if (beet_name_equals_slice(type_decls[i].name, type_decls[i].name_len,
                                 type_decls[j].name,
                                 type_decls[j].name_len)) {
        return 0;
      }
    }
  }

  return 1;
}

static int beet_type_decl_params_are_unique(const beet_ast_type_decl *type_decl) {
  size_t i;
  size_t j;

  assert(type_decl != NULL);

  for (i = 0U; i < type_decl->param_count; ++i) {
    for (j = i + 1U; j < type_decl->param_count; ++j) {
      if (beet_name_equals_slice(type_decl->params[i].name,
                                 type_decl->params[i].name_len,
                                 type_decl->params[j].name,
                                 type_decl->params[j].name_len)) {
        return 0;
      }
    }
  }

  return 1;
}

static int beet_structure_fields_are_unique(const beet_ast_type_decl *type_decl) {
  size_t i;
  size_t j;

  assert(type_decl != NULL);

  for (i = 0U; i < type_decl->field_count; ++i) {
    for (j = i + 1U; j < type_decl->field_count; ++j) {
      if (beet_name_equals_slice(type_decl->fields[i].name,
                                 type_decl->fields[i].name_len,
                                 type_decl->fields[j].name,
                                 type_decl->fields[j].name_len)) {
        return 0;
      }
    }
  }

  return 1;
}

static int beet_choice_variants_are_unique(const beet_ast_type_decl *type_decl) {
  size_t i;
  size_t j;

  assert(type_decl != NULL);

  for (i = 0U; i < type_decl->variant_count; ++i) {
    for (j = i + 1U; j < type_decl->variant_count; ++j) {
      if (beet_name_equals_slice(type_decl->variants[i].name,
                                 type_decl->variants[i].name_len,
                                 type_decl->variants[j].name,
                                 type_decl->variants[j].name_len)) {
        return 0;
      }
    }
  }

  return 1;
}

static int beet_type_check_registered_type_decl(
    const beet_ast_type_decl *type_decl, const beet_ast_type_decl *type_decls,
    size_t decl_count) {
  size_t i;

  assert(type_decl != NULL);
  assert(type_decls != NULL || decl_count == 0U);

  if (type_decl->is_structure == type_decl->is_choice) {
    return 0;
  }

  if (!beet_type_decl_params_are_unique(type_decl)) {
    return 0;
  }

  if (type_decl->is_structure) {
    if (!beet_structure_fields_are_unique(type_decl)) {
      return 0;
    }

    for (i = 0U; i < type_decl->field_count; ++i) {
      if (!beet_type_name_is_known_for_decl(type_decl, type_decls, decl_count,
                                            type_decl->fields[i].type_name,
                                            type_decl->fields[i].type_name_len)) {
        return 0;
      }
    }

    return 1;
  }

  if (!beet_choice_variants_are_unique(type_decl)) {
    return 0;
  }

  for (i = 0U; i < type_decl->variant_count; ++i) {
    if (type_decl->variants[i].has_payload &&
        !beet_type_name_is_known_for_decl(
            type_decl, type_decls, decl_count,
            type_decl->variants[i].payload_type_name,
            type_decl->variants[i].payload_type_name_len)) {
      return 0;
    }
  }

  return 1;
}

int beet_type_check_type_decl(const beet_ast_type_decl *type_decl) {
  assert(type_decl != NULL);

  return beet_type_check_type_decls(type_decl, 1U);
}

int beet_type_check_type_decls(const beet_ast_type_decl *type_decls,
                               size_t decl_count) {
  size_t i;

  assert(type_decls != NULL || decl_count == 0U);

  if (!beet_type_decl_names_are_unique(type_decls, decl_count)) {
    return 0;
  }

  for (i = 0U; i < decl_count; ++i) {
    if (!beet_type_check_registered_type_decl(&type_decls[i], type_decls,
                                              decl_count)) {
      return 0;
    }
  }

  return 1;
}
