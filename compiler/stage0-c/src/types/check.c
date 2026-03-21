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

static beet_type beet_invalid_type(void) {
  beet_type type;

  type.kind = BEET_TYPE_INVALID;
  type.name = NULL;
  type.name_len = 0U;
  return type;
}

static int beet_type_is_valid(const beet_type *type) {
  assert(type != NULL);

  return type->kind != BEET_TYPE_INVALID;
}

static int beet_type_equals(const beet_type *left, const beet_type *right) {
  assert(left != NULL);
  assert(right != NULL);

  if (left->kind != right->kind) {
    return 0;
  }

  if (left->kind != BEET_TYPE_NAMED) {
    return 1;
  }

  return beet_name_equals_slice(left->name, left->name_len, right->name,
                                right->name_len);
}

static const beet_ast_type_decl *
beet_find_type_decl(const beet_ast_type_decl *type_decls, size_t decl_count,
                    const char *name, size_t name_len) {
  size_t i;

  assert(type_decls != NULL || decl_count == 0U);
  assert(name != NULL);

  for (i = 0U; i < decl_count; ++i) {
    if (beet_name_equals_slice(type_decls[i].name, type_decls[i].name_len, name,
                               name_len)) {
      return &type_decls[i];
    }
  }

  return NULL;
}

static const beet_ast_function *
beet_find_function_decl(const beet_ast_function *function_decls,
                        size_t function_count, const char *name,
                        size_t name_len) {
  size_t i;

  assert(function_decls != NULL || function_count == 0U);
  assert(name != NULL);

  for (i = 0U; i < function_count; ++i) {
    if (beet_name_equals_slice(function_decls[i].name,
                               function_decls[i].name_len, name, name_len)) {
      return &function_decls[i];
    }
  }

  return NULL;
}

static beet_type
beet_type_from_name_slice_in_context(const char *name, size_t name_len,
                                     const beet_ast_type_decl *type_decls,
                                     size_t decl_count) {
  beet_type type;
  const beet_ast_type_decl *decl;

  type = beet_type_from_name_slice(name, name_len);
  if (beet_type_is_known(&type)) {
    return type;
  }

  decl = beet_find_type_decl(type_decls, decl_count, name, name_len);
  if (decl == NULL) {
    return beet_invalid_type();
  }

  type.kind = BEET_TYPE_NAMED;
  type.name = decl->name;
  type.name_len = decl->name_len;
  return type;
}

static const beet_ast_field *
beet_find_structure_field(const beet_ast_type_decl *type_decl, const char *name,
                          size_t name_len) {
  size_t i;

  assert(type_decl != NULL);
  assert(name != NULL);

  for (i = 0U; i < type_decl->field_count; ++i) {
    if (beet_name_equals_slice(type_decl->fields[i].name,
                               type_decl->fields[i].name_len, name, name_len)) {
      return &type_decl->fields[i];
    }
  }

  return NULL;
}

static const beet_ast_choice_variant *
beet_find_choice_variant(const beet_ast_type_decl *type_decl, const char *name,
                         size_t name_len) {
  size_t i;

  assert(type_decl != NULL);
  assert(name != NULL);

  for (i = 0U; i < type_decl->variant_count; ++i) {
    if (beet_name_equals_slice(type_decl->variants[i].name,
                               type_decl->variants[i].name_len, name,
                               name_len)) {
      return &type_decl->variants[i];
    }
  }

  return NULL;
}

static beet_type
beet_type_check_expr(const beet_ast_expr *expr, const beet_local_type *locals,
                     size_t local_count, const beet_ast_type_decl *type_decls,
                     size_t decl_count, const beet_ast_function *function_decls,
                     size_t function_count) {
  beet_type type;
  beet_type operand_type;
  beet_type left_type;
  beet_type right_type;
  const beet_local_type *local;

  assert(expr != NULL);
  assert(function_decls != NULL || function_count == 0U);

  type = beet_invalid_type();

  switch (expr->kind) {
  case BEET_AST_EXPR_INT_LITERAL:
    type.kind = BEET_TYPE_INT;
    type.name = "Int";
    type.name_len = 3U;
    return type;

  case BEET_AST_EXPR_BOOL_LITERAL:
    type.kind = BEET_TYPE_BOOL;
    type.name = "Bool";
    type.name_len = 4U;
    return type;

  case BEET_AST_EXPR_NAME:
    local =
        beet_find_local_type(locals, local_count, expr->text, expr->text_len);
    if (local == NULL) {
      return type;
    }
    return local->type;

  case BEET_AST_EXPR_CALL: {
    const beet_ast_function *function_decl;
    size_t i;

    function_decl = beet_find_function_decl(function_decls, function_count,
                                            expr->text, expr->text_len);
    if (function_decl == NULL ||
        function_decl->param_count != expr->arg_count) {
      return type;
    }

    for (i = 0U; i < expr->arg_count; ++i) {
      beet_type param_type;
      beet_type arg_type;

      if (expr->args[i] == NULL) {
        return type;
      }

      param_type = beet_type_from_name_slice_in_context(
          function_decl->params[i].type_name,
          function_decl->params[i].type_name_len, type_decls, decl_count);
      arg_type =
          beet_type_check_expr(expr->args[i], locals, local_count, type_decls,
                               decl_count, function_decls, function_count);
      if (!beet_type_is_valid(&param_type) || !beet_type_is_valid(&arg_type) ||
          !beet_type_equals(&param_type, &arg_type)) {
        return type;
      }
    }

    return beet_type_from_name_slice_in_context(
        function_decl->return_type_name, function_decl->return_type_name_len,
        type_decls, decl_count);
  }

  case BEET_AST_EXPR_CONSTRUCT: {
    const beet_ast_type_decl *type_decl;
    size_t i;
    size_t j;

    type_decl =
        beet_find_type_decl(type_decls, decl_count, expr->text, expr->text_len);
    if (type_decl == NULL) {
      return type;
    }

    if (type_decl->is_structure) {
      if (expr->field_init_count != type_decl->field_count) {
        return type;
      }

      for (i = 0U; i < expr->field_init_count; ++i) {
        const beet_ast_field *field_decl;
        beet_type field_type;
        beet_type value_type;

        for (j = i + 1U; j < expr->field_init_count; ++j) {
          if (beet_name_equals_slice(
                  expr->field_inits[i].name, expr->field_inits[i].name_len,
                  expr->field_inits[j].name, expr->field_inits[j].name_len)) {
            return type;
          }
        }

        field_decl =
            beet_find_structure_field(type_decl, expr->field_inits[i].name,
                                      expr->field_inits[i].name_len);
        if (field_decl == NULL || expr->field_inits[i].value == NULL) {
          return type;
        }

        field_type = beet_type_from_name_slice_in_context(
            field_decl->type_name, field_decl->type_name_len, type_decls,
            decl_count);
        value_type = beet_type_check_expr(expr->field_inits[i].value, locals,
                                          local_count, type_decls, decl_count,
                                          function_decls, function_count);
        if (!beet_type_is_valid(&field_type) ||
            !beet_type_is_valid(&value_type) ||
            !beet_type_equals(&field_type, &value_type)) {
          return type;
        }
      }

      type.kind = BEET_TYPE_NAMED;
      type.name = type_decl->name;
      type.name_len = type_decl->name_len;
      return type;
    }

    if (type_decl->is_choice) {
      const beet_ast_choice_variant *variant_decl;
      beet_type payload_type;
      beet_type value_type;

      if (expr->field_init_count != 1U) {
        return type;
      }

      variant_decl = beet_find_choice_variant(
          type_decl, expr->field_inits[0].name, expr->field_inits[0].name_len);
      if (variant_decl == NULL) {
        return type;
      }

      if (!variant_decl->has_payload) {
        if (expr->field_inits[0].value != NULL) {
          return type;
        }
      } else {
        if (expr->field_inits[0].value == NULL) {
          return type;
        }

        payload_type = beet_type_from_name_slice_in_context(
            variant_decl->payload_type_name,
            variant_decl->payload_type_name_len, type_decls, decl_count);
        value_type = beet_type_check_expr(expr->field_inits[0].value, locals,
                                          local_count, type_decls, decl_count,
                                          function_decls, function_count);
        if (!beet_type_is_valid(&payload_type) ||
            !beet_type_is_valid(&value_type) ||
            !beet_type_equals(&payload_type, &value_type)) {
          return type;
        }
      }

      type.kind = BEET_TYPE_NAMED;
      type.name = type_decl->name;
      type.name_len = type_decl->name_len;
      return type;
    }

    return type;
  }

  case BEET_AST_EXPR_FIELD: {
    const beet_ast_type_decl *type_decl;
    const beet_ast_field *field_decl;

    if (expr->left == NULL) {
      return type;
    }

    left_type =
        beet_type_check_expr(expr->left, locals, local_count, type_decls,
                             decl_count, function_decls, function_count);
    if (left_type.kind != BEET_TYPE_NAMED) {
      return type;
    }

    type_decl = beet_find_type_decl(type_decls, decl_count, left_type.name,
                                    left_type.name_len);
    if (type_decl == NULL || !type_decl->is_structure) {
      return type;
    }

    field_decl =
        beet_find_structure_field(type_decl, expr->text, expr->text_len);
    if (field_decl == NULL) {
      return type;
    }

    return beet_type_from_name_slice_in_context(field_decl->type_name,
                                                field_decl->type_name_len,
                                                type_decls, decl_count);
  }

  case BEET_AST_EXPR_UNARY:
    if (expr->unary_op != BEET_AST_UNARY_NEGATE || expr->left == NULL) {
      return type;
    }

    operand_type =
        beet_type_check_expr(expr->left, locals, local_count, type_decls,
                             decl_count, function_decls, function_count);
    if (operand_type.kind != BEET_TYPE_INT) {
      return type;
    }
    return operand_type;

  case BEET_AST_EXPR_BINARY:
    if (expr->left == NULL || expr->right == NULL) {
      return type;
    }

    left_type =
        beet_type_check_expr(expr->left, locals, local_count, type_decls,
                             decl_count, function_decls, function_count);
    right_type =
        beet_type_check_expr(expr->right, locals, local_count, type_decls,
                             decl_count, function_decls, function_count);
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
      type.name_len = 3U;
      return type;

    case BEET_AST_BINARY_EQ:
    case BEET_AST_BINARY_NE:
    case BEET_AST_BINARY_LT:
    case BEET_AST_BINARY_LE:
    case BEET_AST_BINARY_GT:
    case BEET_AST_BINARY_GE:
      type.kind = BEET_TYPE_BOOL;
      type.name = "Bool";
      type.name_len = 4U;
      return type;

    default:
      return type;
    }

  default:
    return type;
  }
}

static beet_type beet_type_check_binding_in_context(
    const beet_ast_binding *binding, const beet_local_type *locals,
    size_t local_count, const beet_ast_type_decl *type_decls, size_t decl_count,
    const beet_ast_function *function_decls, size_t function_count) {
  assert(binding != NULL);

  return beet_type_check_expr(&binding->expr, locals, local_count, type_decls,
                              decl_count, function_decls, function_count);
}

beet_type beet_type_check_binding(const beet_ast_binding *binding) {
  return beet_type_check_binding_in_context(binding, NULL, 0U, NULL, 0U, NULL,
                                            0U);
}

static beet_type_check_result beet_type_check_binding_annotation_in_context(
    const beet_ast_binding *binding, const beet_local_type *locals,
    size_t local_count, const beet_ast_type_decl *type_decls, size_t decl_count,
    const beet_ast_function *function_decls, size_t function_count) {
  beet_type_check_result result;

  assert(binding != NULL);

  result.ok = 1;
  result.declared_type = beet_invalid_type();
  result.value_type = beet_type_check_binding_in_context(
      binding, locals, local_count, type_decls, decl_count, function_decls,
      function_count);

  if (!binding->has_type) {
    return result;
  }

  result.declared_type = beet_type_from_name_slice_in_context(
      binding->type_name, binding->type_name_len, type_decls, decl_count);

  if (!beet_type_is_valid(&result.declared_type) ||
      !beet_type_is_valid(&result.value_type)) {
    result.ok = 0;
    return result;
  }

  if (!beet_type_equals(&result.declared_type, &result.value_type)) {
    result.ok = 0;
  }

  return result;
}

static int beet_type_check_stmt_list(
    const beet_ast_stmt *stmts, size_t stmt_count, beet_type return_type,
    beet_local_type *locals, size_t *local_count,
    const beet_ast_type_decl *type_decls, size_t decl_count,
    const beet_ast_function *function_decls, size_t function_count) {
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
        result = beet_type_check_binding_annotation_in_context(
            &stmt->binding, locals, *local_count, type_decls, decl_count,
            function_decls, function_count);
        if (!result.ok) {
          return 0;
        }
        binding_type = result.declared_type;
      } else {
        binding_type = beet_type_check_binding_in_context(
            &stmt->binding, locals, *local_count, type_decls, decl_count,
            function_decls, function_count);
        if (!beet_type_is_valid(&binding_type)) {
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

      expr_type =
          beet_type_check_expr(&stmt->expr, locals, *local_count, type_decls,
                               decl_count, function_decls, function_count);
      if (!beet_type_equals(&expr_type, &target->type)) {
        return 0;
      }
      break;
    }

    case BEET_AST_STMT_RETURN: {
      beet_type expr_type =
          beet_type_check_expr(&stmt->expr, locals, *local_count, type_decls,
                               decl_count, function_decls, function_count);
      if (!beet_type_equals(&expr_type, &return_type)) {
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

      condition_type = beet_type_check_expr(
          &stmt->condition, locals, *local_count, type_decls, decl_count,
          function_decls, function_count);
      if (condition_type.kind != BEET_TYPE_BOOL) {
        return 0;
      }

      memcpy(branch_locals, locals, sizeof(beet_local_type) * (*local_count));
      branch_local_count = *local_count;
      if (!beet_type_check_stmt_list(
              stmt->then_body, stmt->then_body_count, return_type,
              branch_locals, &branch_local_count, type_decls, decl_count,
              function_decls, function_count)) {
        return 0;
      }

      memcpy(else_locals, locals, sizeof(beet_local_type) * (*local_count));
      else_local_count = *local_count;
      if (!beet_type_check_stmt_list(stmt->else_body, stmt->else_body_count,
                                     return_type, else_locals,
                                     &else_local_count, type_decls, decl_count,
                                     function_decls, function_count)) {
        return 0;
      }
      break;
    }

    case BEET_AST_STMT_WHILE: {
      beet_type condition_type;
      beet_local_type loop_locals[BEET_TYPE_CHECK_MAX_LOCALS];
      size_t loop_local_count;

      condition_type = beet_type_check_expr(
          &stmt->condition, locals, *local_count, type_decls, decl_count,
          function_decls, function_count);
      if (condition_type.kind != BEET_TYPE_BOOL) {
        return 0;
      }

      memcpy(loop_locals, locals, sizeof(beet_local_type) * (*local_count));
      loop_local_count = *local_count;
      if (!beet_type_check_stmt_list(stmt->loop_body, stmt->loop_body_count,
                                     return_type, loop_locals,
                                     &loop_local_count, type_decls, decl_count,
                                     function_decls, function_count)) {
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
  return beet_type_check_binding_annotation_in_context(binding, NULL, 0U, NULL,
                                                       0U, NULL, 0U);
}

int beet_type_check_function_signature_with_type_decls(
    const beet_ast_function *function_ast, const beet_ast_type_decl *type_decls,
    size_t decl_count) {
  size_t i;
  beet_type return_type;

  assert(function_ast != NULL);
  assert(type_decls != NULL || decl_count == 0U);

  return_type = beet_type_from_name_slice_in_context(
      function_ast->return_type_name, function_ast->return_type_name_len,
      type_decls, decl_count);
  if (!beet_type_is_valid(&return_type)) {
    return 0;
  }

  for (i = 0U; i < function_ast->param_count; ++i) {
    beet_type param_type = beet_type_from_name_slice_in_context(
        function_ast->params[i].type_name,
        function_ast->params[i].type_name_len, type_decls, decl_count);
    if (!beet_type_is_valid(&param_type)) {
      return 0;
    }
  }

  return 1;
}

int beet_type_check_function_signature(const beet_ast_function *function_ast) {
  return beet_type_check_function_signature_with_type_decls(function_ast, NULL,
                                                            0U);
}

int beet_type_check_function_body_with_decls(
    const beet_ast_function *function_ast, const beet_ast_type_decl *type_decls,
    size_t decl_count, const beet_ast_function *function_decls,
    size_t function_count) {
  beet_local_type locals[BEET_TYPE_CHECK_MAX_LOCALS];
  beet_type return_type;
  size_t local_count;
  size_t i;

  assert(function_ast != NULL);
  assert(type_decls != NULL || decl_count == 0U);

  return_type = beet_type_from_name_slice_in_context(
      function_ast->return_type_name, function_ast->return_type_name_len,
      type_decls, decl_count);
  if (!beet_type_is_valid(&return_type)) {
    return 0;
  }

  local_count = 0U;
  for (i = 0U; i < function_ast->param_count; ++i) {
    beet_type param_type = beet_type_from_name_slice_in_context(
        function_ast->params[i].type_name,
        function_ast->params[i].type_name_len, type_decls, decl_count);
    if (!beet_type_is_valid(&param_type) ||
        local_count >= BEET_TYPE_CHECK_MAX_LOCALS) {
      return 0;
    }

    locals[local_count].name = function_ast->params[i].name;
    locals[local_count].name_len = function_ast->params[i].name_len;
    locals[local_count].type = param_type;
    locals[local_count].is_mutable = 0;
    local_count += 1U;
  }

  return beet_type_check_stmt_list(
      function_ast->body, function_ast->body_count, return_type, locals,
      &local_count, type_decls, decl_count, function_decls, function_count);
}

int beet_type_check_function_body_with_type_decls(
    const beet_ast_function *function_ast, const beet_ast_type_decl *type_decls,
    size_t decl_count) {
  return beet_type_check_function_body_with_decls(function_ast, type_decls,
                                                  decl_count, NULL, 0U);
}

int beet_type_check_function_body(const beet_ast_function *function_ast) {
  return beet_type_check_function_body_with_decls(function_ast, NULL, 0U, NULL,
                                                  0U);
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
                               type_decl->params[i].name_len, name, name_len)) {
      return 1;
    }
  }

  return 0;
}

static int beet_type_name_is_known_for_decl(
    const beet_ast_type_decl *type_decl, const beet_ast_type_decl *type_decls,
    size_t decl_count, const char *name, size_t name_len) {
  assert(type_decl != NULL);
  assert(name != NULL);

  return beet_type_name_is_builtin(name, name_len) ||
         beet_type_decl_has_param(type_decl, name, name_len) ||
         beet_find_type_decl(type_decls, decl_count, name, name_len) != NULL;
}

static int beet_type_decl_names_are_unique(const beet_ast_type_decl *type_decls,
                                           size_t decl_count) {
  size_t i;
  size_t j;

  assert(type_decls != NULL || decl_count == 0U);

  for (i = 0U; i < decl_count; ++i) {
    for (j = i + 1U; j < decl_count; ++j) {
      if (beet_name_equals_slice(type_decls[i].name, type_decls[i].name_len,
                                 type_decls[j].name, type_decls[j].name_len)) {
        return 0;
      }
    }
  }

  return 1;
}

static int
beet_type_decl_params_are_unique(const beet_ast_type_decl *type_decl) {
  size_t i;
  size_t j;

  assert(type_decl != NULL);

  for (i = 0U; i < type_decl->param_count; ++i) {
    for (j = i + 1U; j < type_decl->param_count; ++j) {
      if (beet_name_equals_slice(
              type_decl->params[i].name, type_decl->params[i].name_len,
              type_decl->params[j].name, type_decl->params[j].name_len)) {
        return 0;
      }
    }
  }

  return 1;
}

static int
beet_structure_fields_are_unique(const beet_ast_type_decl *type_decl) {
  size_t i;
  size_t j;

  assert(type_decl != NULL);

  for (i = 0U; i < type_decl->field_count; ++i) {
    for (j = i + 1U; j < type_decl->field_count; ++j) {
      if (beet_name_equals_slice(
              type_decl->fields[i].name, type_decl->fields[i].name_len,
              type_decl->fields[j].name, type_decl->fields[j].name_len)) {
        return 0;
      }
    }
  }

  return 1;
}

static int
beet_choice_variants_are_unique(const beet_ast_type_decl *type_decl) {
  size_t i;
  size_t j;

  assert(type_decl != NULL);

  for (i = 0U; i < type_decl->variant_count; ++i) {
    for (j = i + 1U; j < type_decl->variant_count; ++j) {
      if (beet_name_equals_slice(
              type_decl->variants[i].name, type_decl->variants[i].name_len,
              type_decl->variants[j].name, type_decl->variants[j].name_len)) {
        return 0;
      }
    }
  }

  return 1;
}

static int
beet_type_check_registered_type_decl(const beet_ast_type_decl *type_decl,
                                     const beet_ast_type_decl *type_decls,
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
      if (!beet_type_name_is_known_for_decl(
              type_decl, type_decls, decl_count, type_decl->fields[i].type_name,
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
