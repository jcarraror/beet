#include "beet/mir/mir.h"

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>

static void beet_copy_name(char *dst, const char *src, size_t src_len) {
  size_t copy_len;

  assert(dst != NULL);
  assert(src != NULL);

  copy_len = src_len;
  if (copy_len >= BEET_MIR_MAX_NAME_LEN) {
    copy_len = BEET_MIR_MAX_NAME_LEN - 1U;
  }

  memcpy(dst, src, copy_len);
  dst[copy_len] = '\0';
}

static int beet_parse_int_slice(const char *text, size_t len, int *out_value) {
  size_t i;
  int value;

  assert(text != NULL);
  assert(out_value != NULL);

  if (len == 0U) {
    return 0;
  }

  value = 0;
  for (i = 0U; i < len; ++i) {
    if (!isdigit((unsigned char)text[i])) {
      return 0;
    }

    value = (value * 10) + (int)(text[i] - '0');
  }

  *out_value = value;
  return 1;
}

static int beet_name_equals_slice(const char *left, size_t left_len,
                                  const char *right, size_t right_len) {
  return left_len == right_len && strncmp(left, right, left_len) == 0;
}

static const beet_ast_expr *
beet_mir_find_construct_field_value(const beet_ast_expr *construct,
                                    const char *name, size_t name_len) {
  size_t i;

  assert(construct != NULL);
  assert(name != NULL);

  if (construct->kind != BEET_AST_EXPR_CONSTRUCT) {
    return NULL;
  }

  for (i = 0U; i < construct->field_init_count; ++i) {
    if (construct->field_inits[i].value == NULL) {
      continue;
    }

    if (beet_name_equals_slice(construct->field_inits[i].name,
                               construct->field_inits[i].name_len, name,
                               name_len)) {
      return construct->field_inits[i].value;
    }
  }

  return NULL;
}

static const beet_ast_expr *
beet_mir_resolve_field_value_expr(const beet_ast_expr *expr) {
  const beet_ast_expr *base_expr;

  assert(expr != NULL);

  if (expr->kind == BEET_AST_EXPR_CONSTRUCT) {
    return expr;
  }

  if (expr->kind != BEET_AST_EXPR_FIELD || expr->left == NULL) {
    return NULL;
  }

  base_expr = beet_mir_resolve_field_value_expr(expr->left);
  if (base_expr == NULL) {
    return NULL;
  }

  return beet_mir_find_construct_field_value(base_expr, expr->text,
                                             expr->text_len);
}

static int beet_mir_append_name_slice(char *name, size_t *name_len,
                                      const char *text, size_t text_len) {
  assert(name != NULL);
  assert(name_len != NULL);
  assert(text != NULL);

  if (*name_len + text_len >= BEET_MIR_MAX_NAME_LEN) {
    return 0;
  }

  memcpy(name + *name_len, text, text_len);
  *name_len += text_len;
  name[*name_len] = '\0';
  return 1;
}

static int beet_mir_build_field_path(const beet_ast_expr *expr, char *name,
                                     size_t *name_len) {
  assert(expr != NULL);
  assert(name != NULL);
  assert(name_len != NULL);

  switch (expr->kind) {
  case BEET_AST_EXPR_NAME:
    return beet_mir_append_name_slice(name, name_len, expr->text,
                                      expr->text_len);

  case BEET_AST_EXPR_FIELD:
    if (expr->left == NULL) {
      return 0;
    }

    if (!beet_mir_build_field_path(expr->left, name, name_len)) {
      return 0;
    }

    if (!beet_mir_append_name_slice(name, name_len, ".", 1U)) {
      return 0;
    }

    return beet_mir_append_name_slice(name, name_len, expr->text,
                                      expr->text_len);

  default:
    return 0;
  }
}

static int beet_mir_build_field_name(char *name, const char *base_name,
                                     size_t base_name_len,
                                     const char *field_name,
                                     size_t field_name_len) {
  size_t name_len;

  assert(name != NULL);
  assert(base_name != NULL);
  assert(field_name != NULL);

  name_len = 0U;
  name[0] = '\0';

  if (!beet_mir_append_name_slice(name, &name_len, base_name, base_name_len)) {
    return 0;
  }

  if (!beet_mir_append_name_slice(name, &name_len, ".", 1U)) {
    return 0;
  }

  return beet_mir_append_name_slice(name, &name_len, field_name,
                                    field_name_len);
}

void beet_mir_function_init(beet_mir_function *function, const char *name,
                            size_t name_len) {
  assert(function != NULL);
  assert(name != NULL);

  beet_copy_name(function->name, name, name_len);
  function->instr_count = 0U;
  function->local_count = 0U;
  function->param_count = 0U;
  function->next_temp = 0;
  function->next_label = 0;
}

int beet_mir_add_const_int(beet_mir_function *function, int value) {
  beet_mir_instr *instr;
  int temp;

  assert(function != NULL);

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return -1;
  }

  temp = function->next_temp;
  function->next_temp += 1;

  instr = &function->instrs[function->instr_count];
  instr->op = BEET_MIR_OP_CONST_INT;
  instr->dst = temp;
  instr->src_lhs = -1;
  instr->src_rhs = -1;
  instr->int_value = value;
  instr->name[0] = '\0';
  instr->arg_count = 0U;

  function->instr_count += 1U;
  return temp;
}

int beet_mir_add_bind_local(beet_mir_function *function, const char *name,
                            size_t name_len, int src_temp) {
  beet_mir_instr *instr;

  assert(function != NULL);
  assert(name != NULL);

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return 0;
  }

  if (function->local_count >= BEET_MIR_MAX_LOCALS) {
    return 0;
  }

  instr = &function->instrs[function->instr_count];
  instr->op = BEET_MIR_OP_BIND_LOCAL;
  instr->dst = src_temp;
  instr->src_lhs = -1;
  instr->src_rhs = -1;
  instr->int_value = 0;
  beet_copy_name(instr->name, name, name_len);
  instr->arg_count = 0U;

  beet_copy_name(function->locals[function->local_count], name, name_len);
  function->local_count += 1U;
  function->instr_count += 1U;
  return 1;
}

int beet_mir_add_load_local(beet_mir_function *function, const char *name,
                            size_t name_len) {
  beet_mir_instr *instr;
  int temp;

  assert(function != NULL);
  assert(name != NULL);

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return -1;
  }

  temp = function->next_temp;
  function->next_temp += 1;

  instr = &function->instrs[function->instr_count];
  instr->op = BEET_MIR_OP_LOAD_LOCAL;
  instr->dst = temp;
  instr->src_lhs = -1;
  instr->src_rhs = -1;
  instr->int_value = 0;
  beet_copy_name(instr->name, name, name_len);
  instr->arg_count = 0U;

  function->instr_count += 1U;
  return temp;
}

int beet_mir_add_store_local(beet_mir_function *function, const char *name,
                             size_t name_len, int src_temp) {
  beet_mir_instr *instr;

  assert(function != NULL);
  assert(name != NULL);

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return 0;
  }

  instr = &function->instrs[function->instr_count];
  instr->op = BEET_MIR_OP_STORE_LOCAL;
  instr->dst = src_temp;
  instr->src_lhs = -1;
  instr->src_rhs = -1;
  instr->int_value = 0;
  beet_copy_name(instr->name, name, name_len);
  instr->arg_count = 0U;

  function->instr_count += 1U;
  return 1;
}

int beet_mir_add_call(beet_mir_function *function, const char *name,
                      size_t name_len, const int *args, size_t arg_count) {
  beet_mir_instr *instr;
  size_t i;
  int temp;

  assert(function != NULL);
  assert(name != NULL);
  assert(args != NULL || arg_count == 0U);

  if (arg_count > BEET_AST_MAX_PARAMS) {
    return -1;
  }

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return -1;
  }

  temp = function->next_temp;
  function->next_temp += 1;

  instr = &function->instrs[function->instr_count];
  instr->op = BEET_MIR_OP_CALL;
  instr->dst = temp;
  instr->src_lhs = -1;
  instr->src_rhs = -1;
  instr->int_value = 0;
  beet_copy_name(instr->name, name, name_len);
  instr->arg_count = arg_count;

  for (i = 0U; i < arg_count; ++i) {
    instr->args[i] = args[i];
  }

  function->instr_count += 1U;
  return temp;
}

int beet_mir_add_binary_int(beet_mir_function *function, beet_mir_opcode op,
                            int lhs_temp, int rhs_temp) {
  beet_mir_instr *instr;
  int temp;

  assert(function != NULL);

  if (op != BEET_MIR_OP_ADD_INT && op != BEET_MIR_OP_SUB_INT &&
      op != BEET_MIR_OP_MUL_INT && op != BEET_MIR_OP_DIV_INT &&
      op != BEET_MIR_OP_EQ_INT && op != BEET_MIR_OP_NE_INT &&
      op != BEET_MIR_OP_LT_INT && op != BEET_MIR_OP_LE_INT &&
      op != BEET_MIR_OP_GT_INT && op != BEET_MIR_OP_GE_INT) {
    return -1;
  }

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return -1;
  }

  temp = function->next_temp;
  function->next_temp += 1;

  instr = &function->instrs[function->instr_count];
  instr->op = op;
  instr->dst = temp;
  instr->src_lhs = lhs_temp;
  instr->src_rhs = rhs_temp;
  instr->int_value = 0;
  instr->name[0] = '\0';
  instr->arg_count = 0U;

  function->instr_count += 1U;
  return temp;
}

int beet_mir_next_label(beet_mir_function *function) {
  int label_id;

  assert(function != NULL);

  label_id = function->next_label;
  function->next_label += 1;
  return label_id;
}

int beet_mir_add_label(beet_mir_function *function, int label_id) {
  beet_mir_instr *instr;

  assert(function != NULL);

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return 0;
  }

  instr = &function->instrs[function->instr_count];
  instr->op = BEET_MIR_OP_LABEL;
  instr->dst = -1;
  instr->src_lhs = -1;
  instr->src_rhs = -1;
  instr->int_value = label_id;
  instr->name[0] = '\0';
  instr->arg_count = 0U;

  function->instr_count += 1U;
  return 1;
}

int beet_mir_add_jump(beet_mir_function *function, int label_id) {
  beet_mir_instr *instr;

  assert(function != NULL);

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return 0;
  }

  instr = &function->instrs[function->instr_count];
  instr->op = BEET_MIR_OP_JUMP;
  instr->dst = -1;
  instr->src_lhs = -1;
  instr->src_rhs = -1;
  instr->int_value = label_id;
  instr->name[0] = '\0';

  function->instr_count += 1U;
  return 1;
}

int beet_mir_add_jump_if_false(beet_mir_function *function, int condition_temp,
                               int label_id) {
  beet_mir_instr *instr;

  assert(function != NULL);

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return 0;
  }

  instr = &function->instrs[function->instr_count];
  instr->op = BEET_MIR_OP_JUMP_IF_FALSE;
  instr->dst = condition_temp;
  instr->src_lhs = -1;
  instr->src_rhs = -1;
  instr->int_value = label_id;
  instr->name[0] = '\0';

  function->instr_count += 1U;
  return 1;
}

int beet_mir_add_return_local(beet_mir_function *function, const char *name,
                              size_t name_len) {
  beet_mir_instr *instr;

  assert(function != NULL);
  assert(name != NULL);

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return 0;
  }

  instr = &function->instrs[function->instr_count];
  instr->op = BEET_MIR_OP_RETURN_LOCAL;
  instr->dst = -1;
  instr->src_lhs = -1;
  instr->src_rhs = -1;
  instr->int_value = 0;
  beet_copy_name(instr->name, name, name_len);
  instr->arg_count = 0U;

  function->instr_count += 1U;
  return 1;
}

int beet_mir_add_return_temp(beet_mir_function *function, int temp) {
  beet_mir_instr *instr;

  assert(function != NULL);

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return 0;
  }

  instr = &function->instrs[function->instr_count];
  instr->op = BEET_MIR_OP_RETURN_TEMP;
  instr->dst = temp;
  instr->src_lhs = -1;
  instr->src_rhs = -1;
  instr->int_value = 0;
  instr->name[0] = '\0';
  instr->arg_count = 0U;

  function->instr_count += 1U;
  return 1;
}

int beet_mir_add_return_const_int(beet_mir_function *function, int value) {
  beet_mir_instr *instr;

  assert(function != NULL);

  if (function->instr_count >= BEET_MIR_MAX_INSTRS) {
    return 0;
  }

  instr = &function->instrs[function->instr_count];
  instr->op = BEET_MIR_OP_RETURN_CONST_INT;
  instr->dst = -1;
  instr->src_lhs = -1;
  instr->src_rhs = -1;
  instr->int_value = value;
  instr->name[0] = '\0';
  instr->arg_count = 0U;

  function->instr_count += 1U;
  return 1;
}

static int beet_mir_lower_expr(beet_mir_function *function,
                               const beet_ast_expr *expr, int *out_temp);

static int beet_mir_lower_expr_into_local(beet_mir_function *function,
                                          const char *name, size_t name_len,
                                          const beet_ast_expr *expr) {
  char field_name[BEET_MIR_MAX_NAME_LEN];
  int temp;
  size_t i;

  assert(function != NULL);
  assert(name != NULL);
  assert(expr != NULL);

  if (expr->kind == BEET_AST_EXPR_CONSTRUCT) {
    for (i = 0U; i < expr->field_init_count; ++i) {
      if (expr->field_inits[i].value == NULL) {
        return 0;
      }

      if (!beet_mir_build_field_name(field_name, name, name_len,
                                     expr->field_inits[i].name,
                                     expr->field_inits[i].name_len)) {
        return 0;
      }

      if (!beet_mir_lower_expr_into_local(function, field_name,
                                          strlen(field_name),
                                          expr->field_inits[i].value)) {
        return 0;
      }
    }

    return 1;
  }

  if (!beet_mir_lower_expr(function, expr, &temp)) {
    return 0;
  }

  return beet_mir_add_bind_local(function, name, name_len, temp);
}

int beet_mir_lower_binding(beet_mir_function *function,
                           const beet_ast_binding *binding) {
  assert(function != NULL);
  assert(binding != NULL);

  return beet_mir_lower_expr_into_local(function, binding->name,
                                        binding->name_len, &binding->expr);
}

static int
beet_mir_register_param_locals(beet_mir_function *function,
                               const beet_ast_function *function_ast) {
  size_t i;

  assert(function != NULL);
  assert(function_ast != NULL);

  function->param_count = function_ast->param_count;

  for (i = 0U; i < function_ast->param_count; ++i) {
    if (function->local_count >= BEET_MIR_MAX_LOCALS) {
      return 0;
    }

    beet_copy_name(function->locals[function->local_count],
                   function_ast->params[i].name,
                   function_ast->params[i].name_len);
    function->local_count += 1U;
  }

  return 1;
}

static int beet_mir_lower_expr(beet_mir_function *function,
                               const beet_ast_expr *expr, int *out_temp) {
  char local_name[BEET_MIR_MAX_NAME_LEN];
  const beet_ast_expr *value_expr;
  size_t local_name_len;
  int lhs_temp;
  int rhs_temp;
  int arg_temps[BEET_AST_MAX_PARAMS];
  size_t i;

  assert(function != NULL);
  assert(expr != NULL);
  assert(out_temp != NULL);

  switch (expr->kind) {
  case BEET_AST_EXPR_INT_LITERAL:
    if (!beet_parse_int_slice(expr->text, expr->text_len, &lhs_temp)) {
      return 0;
    }

    *out_temp = beet_mir_add_const_int(function, lhs_temp);
    return *out_temp >= 0;

  case BEET_AST_EXPR_BOOL_LITERAL:
    if (expr->text_len == 4U) {
      *out_temp = beet_mir_add_const_int(function, 1);
      return *out_temp >= 0;
    }
    if (expr->text_len == 5U) {
      *out_temp = beet_mir_add_const_int(function, 0);
      return *out_temp >= 0;
    }
    return 0;

  case BEET_AST_EXPR_NAME:
    *out_temp = beet_mir_add_load_local(function, expr->text, expr->text_len);
    return *out_temp >= 0;

  case BEET_AST_EXPR_CALL:
    if (expr->arg_count > BEET_AST_MAX_PARAMS) {
      return 0;
    }

    for (i = 0U; i < expr->arg_count; ++i) {
      if (expr->args[i] == NULL ||
          !beet_mir_lower_expr(function, expr->args[i], &arg_temps[i])) {
        return 0;
      }
    }

    *out_temp = beet_mir_add_call(function, expr->text, expr->text_len,
                                  arg_temps, expr->arg_count);
    return *out_temp >= 0;

  case BEET_AST_EXPR_FIELD:
    value_expr = beet_mir_resolve_field_value_expr(expr);
    if (value_expr != NULL) {
      return beet_mir_lower_expr(function, value_expr, out_temp);
    }

    local_name[0] = '\0';
    local_name_len = 0U;
    if (!beet_mir_build_field_path(expr, local_name, &local_name_len)) {
      return 0;
    }

    *out_temp = beet_mir_add_load_local(function, local_name, local_name_len);
    return *out_temp >= 0;

  case BEET_AST_EXPR_UNARY: {
    int zero_temp;

    if (expr->unary_op != BEET_AST_UNARY_NEGATE || expr->left == NULL) {
      return 0;
    }

    zero_temp = beet_mir_add_const_int(function, 0);
    if (zero_temp < 0) {
      return 0;
    }

    if (!beet_mir_lower_expr(function, expr->left, &rhs_temp)) {
      return 0;
    }

    *out_temp = beet_mir_add_binary_int(function, BEET_MIR_OP_SUB_INT,
                                        zero_temp, rhs_temp);
    return *out_temp >= 0;
  }

  case BEET_AST_EXPR_BINARY:
    if (expr->left == NULL || expr->right == NULL) {
      return 0;
    }

    if (!beet_mir_lower_expr(function, expr->left, &lhs_temp)) {
      return 0;
    }

    if (!beet_mir_lower_expr(function, expr->right, &rhs_temp)) {
      return 0;
    }

    switch (expr->binary_op) {
    case BEET_AST_BINARY_ADD:
      *out_temp = beet_mir_add_binary_int(function, BEET_MIR_OP_ADD_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_SUB:
      *out_temp = beet_mir_add_binary_int(function, BEET_MIR_OP_SUB_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_MUL:
      *out_temp = beet_mir_add_binary_int(function, BEET_MIR_OP_MUL_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_DIV:
      *out_temp = beet_mir_add_binary_int(function, BEET_MIR_OP_DIV_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_EQ:
      *out_temp = beet_mir_add_binary_int(function, BEET_MIR_OP_EQ_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_NE:
      *out_temp = beet_mir_add_binary_int(function, BEET_MIR_OP_NE_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_LT:
      *out_temp = beet_mir_add_binary_int(function, BEET_MIR_OP_LT_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_LE:
      *out_temp = beet_mir_add_binary_int(function, BEET_MIR_OP_LE_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_GT:
      *out_temp = beet_mir_add_binary_int(function, BEET_MIR_OP_GT_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_GE:
      *out_temp = beet_mir_add_binary_int(function, BEET_MIR_OP_GE_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    default:
      return 0;
    }

  default:
    return 0;
  }
}

static int beet_mir_lower_stmt_list(beet_mir_function *function,
                                    const beet_ast_stmt *stmts,
                                    size_t stmt_count);

static int beet_mir_lower_return_stmt(beet_mir_function *function,
                                      const beet_ast_stmt *stmt) {
  int value;
  int temp;

  assert(function != NULL);
  assert(stmt != NULL);
  assert(stmt->kind == BEET_AST_STMT_RETURN);

  switch (stmt->expr.kind) {
  case BEET_AST_EXPR_INT_LITERAL:
    if (!beet_parse_int_slice(stmt->expr.text, stmt->expr.text_len, &value)) {
      return 0;
    }
    return beet_mir_add_return_const_int(function, value);

  case BEET_AST_EXPR_BOOL_LITERAL:
    if (stmt->expr.text_len == 4U) {
      return beet_mir_add_return_const_int(function, 1);
    }
    if (stmt->expr.text_len == 5U) {
      return beet_mir_add_return_const_int(function, 0);
    }
    return 0;

  case BEET_AST_EXPR_NAME:
    return beet_mir_add_return_local(function, stmt->expr.text,
                                     stmt->expr.text_len);

  default:
    if (!beet_mir_lower_expr(function, &stmt->expr, &temp)) {
      return 0;
    }
    return beet_mir_add_return_temp(function, temp);
  }
}

static int beet_mir_lower_while_stmt(beet_mir_function *function,
                                     const beet_ast_stmt *stmt) {
  int start_label;
  int end_label;
  int condition_temp;

  assert(function != NULL);
  assert(stmt != NULL);
  assert(stmt->kind == BEET_AST_STMT_WHILE);

  start_label = beet_mir_next_label(function);
  end_label = beet_mir_next_label(function);

  if (!beet_mir_add_label(function, start_label)) {
    return 0;
  }

  if (!beet_mir_lower_expr(function, &stmt->condition, &condition_temp)) {
    return 0;
  }

  if (!beet_mir_add_jump_if_false(function, condition_temp, end_label)) {
    return 0;
  }

  if (!beet_mir_lower_stmt_list(function, stmt->loop_body,
                                stmt->loop_body_count)) {
    return 0;
  }

  if (!beet_mir_add_jump(function, start_label)) {
    return 0;
  }

  return beet_mir_add_label(function, end_label);
}

static int beet_mir_lower_if_stmt(beet_mir_function *function,
                                  const beet_ast_stmt *stmt) {
  int condition_temp;
  int else_label;
  int end_label;

  assert(function != NULL);
  assert(stmt != NULL);
  assert(stmt->kind == BEET_AST_STMT_IF);

  if (!beet_mir_lower_expr(function, &stmt->condition, &condition_temp)) {
    return 0;
  }

  else_label = beet_mir_next_label(function);
  if (!beet_mir_add_jump_if_false(function, condition_temp, else_label)) {
    return 0;
  }

  if (!beet_mir_lower_stmt_list(function, stmt->then_body,
                                stmt->then_body_count)) {
    return 0;
  }

  if (stmt->else_body_count == 0U) {
    return beet_mir_add_label(function, else_label);
  }

  end_label = beet_mir_next_label(function);
  if (!beet_mir_add_jump(function, end_label)) {
    return 0;
  }

  if (!beet_mir_add_label(function, else_label)) {
    return 0;
  }

  if (!beet_mir_lower_stmt_list(function, stmt->else_body,
                                stmt->else_body_count)) {
    return 0;
  }

  return beet_mir_add_label(function, end_label);
}

static int beet_mir_lower_stmt_list(beet_mir_function *function,
                                    const beet_ast_stmt *stmts,
                                    size_t stmt_count) {
  size_t i;

  assert(function != NULL);
  assert(stmts != NULL || stmt_count == 0U);

  for (i = 0U; i < stmt_count; ++i) {
    const beet_ast_stmt *stmt = &stmts[i];

    switch (stmt->kind) {
    case BEET_AST_STMT_BINDING:
      if (!beet_mir_lower_binding(function, &stmt->binding)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_ASSIGNMENT: {
      int temp;

      if (!beet_mir_lower_expr(function, &stmt->expr, &temp)) {
        return 0;
      }

      if (!beet_mir_add_store_local(function, stmt->assignment.name,
                                    stmt->assignment.name_len, temp)) {
        return 0;
      }
      break;
    }

    case BEET_AST_STMT_RETURN:
      if (!beet_mir_lower_return_stmt(function, stmt)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_IF:
      if (!beet_mir_lower_if_stmt(function, stmt)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_WHILE:
      if (!beet_mir_lower_while_stmt(function, stmt)) {
        return 0;
      }
      break;

    default:
      return 0;
    }
  }

  return 1;
}

int beet_mir_lower_trivial_return_function(
    beet_mir_function *function, const beet_ast_function *function_ast,
    size_t entry_index) {
  (void)entry_index;

  return beet_mir_lower_function(function, function_ast);
}

int beet_mir_lower_function(beet_mir_function *function,
                            const beet_ast_function *function_ast) {
  assert(function != NULL);
  assert(function_ast != NULL);

  beet_mir_function_init(function, function_ast->name, function_ast->name_len);
  if (!beet_mir_register_param_locals(function, function_ast)) {
    return 0;
  }

  return beet_mir_lower_stmt_list(function, function_ast->body,
                                  function_ast->body_count);
}
