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
  instr->int_value = 0;
  beet_copy_name(instr->name, name, name_len);

  beet_copy_name(function->locals[function->local_count], name, name_len);
  function->local_count += 1U;
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
  instr->int_value = 0;
  beet_copy_name(instr->name, name, name_len);

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

static int beet_mir_lower_return_stmt(beet_mir_function *function,
                                      const beet_ast_stmt *stmt) {
  int value;

  assert(function != NULL);
  assert(stmt != NULL);
  assert(stmt->kind == BEET_AST_STMT_RETURN);

  switch (stmt->expr.kind) {
  case BEET_AST_EXPR_INT_LITERAL:
    if (!beet_parse_int_slice(stmt->expr.text, stmt->expr.text_len, &value)) {
      return 0;
    }
    return beet_mir_add_return_const_int(function, value);

  case BEET_AST_EXPR_NAME:
    return beet_mir_add_return_local(function, stmt->expr.text,
                                     stmt->expr.text_len);

  default:
    return 0;
  }
}

int beet_mir_lower_trivial_return_function(
    beet_mir_function *function, const beet_ast_function *function_ast,
    size_t entry_index) {
  (void)entry_index;

  return beet_mir_lower_function(function, function_ast);
}

int beet_mir_lower_function(beet_mir_function *function,
                            const beet_ast_function *function_ast) {
  size_t i;

  assert(function != NULL);
  assert(function_ast != NULL);

  beet_mir_function_init(function, function_ast->name, function_ast->name_len);

  for (i = 0U; i < function_ast->body_count; ++i) {
    const beet_ast_stmt *stmt = &function_ast->body[i];

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

    default:
      return 0;
    }
  }

  return 1;
}