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

void beet_mir_function_init(beet_mir_function *function, const char *name,
                            size_t name_len) {
  assert(function != NULL);
  assert(name != NULL);

  beet_copy_name(function->name, name, name_len);
  function->instr_count = 0U;
  function->local_count = 0U;
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

  function->instr_count += 1U;
  return 1;
}

int beet_mir_lower_binding(beet_mir_function *function,
                           const beet_ast_binding *binding) {
  int value;
  int temp;

  assert(function != NULL);
  assert(binding != NULL);

  if (!beet_parse_int_slice(binding->value_text, binding->value_len, &value)) {
    return 0;
  }

  temp = beet_mir_add_const_int(function, value);
  if (temp < 0) {
    return 0;
  }

  if (!beet_mir_add_bind_local(function, binding->name, binding->name_len,
                               temp)) {
    return 0;
  }

  return 1;
}

static int
beet_mir_register_param_locals(beet_mir_function *function,
                               const beet_ast_function *function_ast) {
  size_t i;

  assert(function != NULL);
  assert(function_ast != NULL);

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
  int lhs_temp;
  int rhs_temp;

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

  case BEET_AST_EXPR_UNARY:
  case BEET_AST_EXPR_BINARY:
    if (!beet_mir_lower_expr(function, &stmt->expr, &temp)) {
      return 0;
    }
    return beet_mir_add_return_temp(function, temp);

  default:
    return 0;
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
