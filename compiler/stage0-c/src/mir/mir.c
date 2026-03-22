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

typedef struct beet_mir_local_type {
  const char *name;
  size_t name_len;
  const char *type_name;
  size_t type_name_len;
  const beet_ast_type_decl *type_decl;
} beet_mir_local_type;

typedef struct beet_mir_lower_context {
  beet_mir_function *function;
  const beet_ast_type_decl *type_decls;
  size_t decl_count;
  beet_mir_local_type locals[BEET_MIR_MAX_LOCALS];
  size_t local_count;
} beet_mir_lower_context;

static const beet_ast_type_decl *
beet_mir_find_type_decl(const beet_ast_type_decl *type_decls, size_t decl_count,
                        const char *name, size_t name_len) {
  beet_decl_registry registry;

  beet_decl_registry_init(&registry, type_decls, decl_count, NULL, 0U);
  return beet_decl_registry_find_type(&registry, name, name_len);
}

static int
beet_mir_context_bind_local_type(beet_mir_lower_context *ctx, const char *name,
                                 size_t name_len, const char *type_name,
                                 size_t type_name_len,
                                 const beet_ast_type_decl *type_decl) {
  assert(ctx != NULL);
  assert(name != NULL);
  assert(type_name != NULL);

  if (type_decl == NULL) {
    type_decl = beet_mir_find_type_decl(ctx->type_decls, ctx->decl_count,
                                        type_name, type_name_len);
  }

  if (ctx->local_count >= BEET_MIR_MAX_LOCALS) {
    return 0;
  }

  ctx->locals[ctx->local_count].name = name;
  ctx->locals[ctx->local_count].name_len = name_len;
  ctx->locals[ctx->local_count].type_name = type_name;
  ctx->locals[ctx->local_count].type_name_len = type_name_len;
  ctx->locals[ctx->local_count].type_decl = type_decl;
  ctx->local_count += 1U;
  return 1;
}

static int beet_mir_function_has_local(const beet_mir_function *function,
                                       const char *name, size_t name_len) {
  size_t i;

  assert(function != NULL);
  assert(name != NULL);

  for (i = 0U; i < function->local_count; ++i) {
    if (strncmp(function->locals[i], name, name_len) == 0 &&
        function->locals[i][name_len] == '\0') {
      return 1;
    }
  }

  return 0;
}

static int beet_mir_reserve_local(beet_mir_function *function, const char *name,
                                  size_t name_len) {
  assert(function != NULL);
  assert(name != NULL);

  if (beet_mir_function_has_local(function, name, name_len)) {
    return 1;
  }

  if (function->local_count >= BEET_MIR_MAX_LOCALS) {
    return 0;
  }

  beet_copy_name(function->locals[function->local_count], name, name_len);
  function->local_count += 1U;
  return 1;
}

static const beet_mir_local_type *
beet_mir_context_find_local_type(const beet_mir_lower_context *ctx,
                                 const char *name, size_t name_len) {
  size_t i;

  assert(ctx != NULL);
  assert(name != NULL);

  i = ctx->local_count;
  while (i > 0U) {
    i -= 1U;
    if (beet_name_equals_slice(ctx->locals[i].name, ctx->locals[i].name_len,
                               name, name_len)) {
      return &ctx->locals[i];
    }
  }

  return NULL;
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

  if (!beet_mir_function_has_local(function, name, name_len) &&
      function->local_count >= BEET_MIR_MAX_LOCALS) {
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

  if (!beet_mir_reserve_local(function, name, name_len)) {
    return 0;
  }
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

static int beet_mir_lower_expr(beet_mir_lower_context *ctx,
                               const beet_ast_expr *expr, int *out_temp);

static int beet_mir_build_choice_tag_name(char *name, const char *base_name,
                                          size_t base_name_len) {
  assert(name != NULL);
  assert(base_name != NULL);

  return beet_mir_build_field_name(name, base_name, base_name_len, "tag", 3U);
}

static int beet_mir_build_choice_variant_storage_name(char *name,
                                                      const char *base_name,
                                                      size_t base_name_len,
                                                      const char *variant_name,
                                                      size_t variant_name_len) {
  assert(name != NULL);
  assert(base_name != NULL);
  assert(variant_name != NULL);

  return beet_mir_build_field_name(name, base_name, base_name_len, variant_name,
                                   variant_name_len);
}

static int beet_mir_reserve_typed_local_storage(
    beet_mir_lower_context *ctx, const char *name, size_t name_len,
    const char *type_name, size_t type_name_len,
    const beet_ast_type_decl *type_decl) {
  size_t i;

  assert(ctx != NULL);
  assert(name != NULL);
  assert(type_name != NULL);

  if (type_decl == NULL && type_name != NULL) {
    type_decl = beet_mir_find_type_decl(ctx->type_decls, ctx->decl_count,
                                        type_name, type_name_len);
  }

  if (type_decl == NULL) {
    return beet_mir_reserve_local(ctx->function, name, name_len);
  }

  if (type_decl->is_choice) {
    char field_name[BEET_MIR_MAX_NAME_LEN];

    if (!beet_mir_build_choice_tag_name(field_name, name, name_len) ||
        !beet_mir_reserve_local(ctx->function, field_name,
                                strlen(field_name))) {
      return 0;
    }

    for (i = 0U; i < type_decl->variant_count; ++i) {
      if (!type_decl->variants[i].has_payload) {
        continue;
      }

      if (!beet_mir_build_choice_variant_storage_name(
              field_name, name, name_len, type_decl->variants[i].name,
              type_decl->variants[i].name_len) ||
          !beet_mir_reserve_typed_local_storage(
              ctx, field_name, strlen(field_name),
              type_decl->variants[i].payload_type_name,
              type_decl->variants[i].payload_type_name_len, NULL)) {
        return 0;
      }
    }

    return 1;
  }

  for (i = 0U; i < type_decl->field_count; ++i) {
    char field_name[BEET_MIR_MAX_NAME_LEN];

    if (!beet_mir_build_field_name(field_name, name, name_len,
                                   type_decl->fields[i].name,
                                   type_decl->fields[i].name_len) ||
        !beet_mir_reserve_typed_local_storage(
            ctx, field_name, strlen(field_name), type_decl->fields[i].type_name,
            type_decl->fields[i].type_name_len, NULL)) {
      return 0;
    }
  }

  return 1;
}

static int beet_mir_copy_local_value(beet_mir_lower_context *ctx,
                                     const char *dst_name, size_t dst_name_len,
                                     const char *src_name, size_t src_name_len,
                                     const char *type_name,
                                     size_t type_name_len,
                                     const beet_ast_type_decl *type_decl) {
  char dst_field_name[BEET_MIR_MAX_NAME_LEN];
  char src_field_name[BEET_MIR_MAX_NAME_LEN];
  int temp;
  size_t i;

  assert(ctx != NULL);
  assert(dst_name != NULL);
  assert(src_name != NULL);
  assert(type_name != NULL);

  if (type_decl == NULL && type_name != NULL) {
    type_decl = beet_mir_find_type_decl(ctx->type_decls, ctx->decl_count,
                                        type_name, type_name_len);
  }

  if (type_decl == NULL) {
    temp = beet_mir_add_load_local(ctx->function, src_name, src_name_len);
    if (temp < 0) {
      return 0;
    }
    return beet_mir_add_bind_local(ctx->function, dst_name, dst_name_len, temp);
  }

  if (type_decl->is_structure) {
    for (i = 0U; i < type_decl->field_count; ++i) {
      if (!beet_mir_build_field_name(dst_field_name, dst_name, dst_name_len,
                                     type_decl->fields[i].name,
                                     type_decl->fields[i].name_len)) {
        return 0;
      }
      if (!beet_mir_build_field_name(src_field_name, src_name, src_name_len,
                                     type_decl->fields[i].name,
                                     type_decl->fields[i].name_len)) {
        return 0;
      }
      if (!beet_mir_copy_local_value(
              ctx, dst_field_name, strlen(dst_field_name), src_field_name,
              strlen(src_field_name), type_decl->fields[i].type_name,
              type_decl->fields[i].type_name_len, NULL)) {
        return 0;
      }
    }
    return 1;
  }

  if (type_decl->is_choice) {
    if (!beet_mir_build_choice_tag_name(dst_field_name, dst_name,
                                        dst_name_len)) {
      return 0;
    }
    if (!beet_mir_build_choice_tag_name(src_field_name, src_name,
                                        src_name_len)) {
      return 0;
    }
    temp = beet_mir_add_load_local(ctx->function, src_field_name,
                                   strlen(src_field_name));
    if (temp < 0 || !beet_mir_add_bind_local(ctx->function, dst_field_name,
                                             strlen(dst_field_name), temp)) {
      return 0;
    }

    for (i = 0U; i < type_decl->variant_count; ++i) {
      if (!type_decl->variants[i].has_payload) {
        continue;
      }
      if (!beet_mir_build_choice_variant_storage_name(
              dst_field_name, dst_name, dst_name_len,
              type_decl->variants[i].name, type_decl->variants[i].name_len)) {
        return 0;
      }
      if (!beet_mir_build_choice_variant_storage_name(
              src_field_name, src_name, src_name_len,
              type_decl->variants[i].name, type_decl->variants[i].name_len)) {
        return 0;
      }
      if (!beet_mir_copy_local_value(
              ctx, dst_field_name, strlen(dst_field_name), src_field_name,
              strlen(src_field_name), type_decl->variants[i].payload_type_name,
              type_decl->variants[i].payload_type_name_len, NULL)) {
        return 0;
      }
    }
    return 1;
  }

  return 0;
}

static int beet_mir_lower_expr_into_local(beet_mir_lower_context *ctx,
                                          const char *name, size_t name_len,
                                          const beet_ast_expr *expr) {
  const beet_ast_type_decl *type_decl;
  char field_name[BEET_MIR_MAX_NAME_LEN];
  int temp;
  size_t i;
  size_t variant_index;

  assert(ctx != NULL);
  assert(name != NULL);
  assert(expr != NULL);

  if (expr->kind == BEET_AST_EXPR_CONSTRUCT) {
    type_decl = expr->resolved_type_decl;
    if (type_decl != NULL &&
        !beet_mir_reserve_typed_local_storage(ctx, name, name_len, expr->text,
                                              expr->text_len, type_decl)) {
      return 0;
    }

    if (type_decl != NULL && type_decl->is_choice) {
      if (expr->field_init_count != 1U || expr->resolved_variant_decl == NULL) {
        return 0;
      }
      variant_index = expr->resolved_variant_index;

      if (!beet_mir_build_choice_tag_name(field_name, name, name_len)) {
        return 0;
      }
      temp = beet_mir_add_const_int(ctx->function, (int)variant_index);
      if (temp < 0 || !beet_mir_add_bind_local(ctx->function, field_name,
                                               strlen(field_name), temp)) {
        return 0;
      }

      if (expr->field_inits[0].value == NULL) {
        return 1;
      }

      if (!beet_mir_build_choice_variant_storage_name(
              field_name, name, name_len, expr->field_inits[0].name,
              expr->field_inits[0].name_len)) {
        return 0;
      }
      return beet_mir_lower_expr_into_local(ctx, field_name, strlen(field_name),
                                            expr->field_inits[0].value);
    }

    for (i = 0U; i < expr->field_init_count; ++i) {
      if (expr->field_inits[i].value == NULL) {
        return 0;
      }

      if (!beet_mir_build_field_name(field_name, name, name_len,
                                     expr->field_inits[i].name,
                                     expr->field_inits[i].name_len)) {
        return 0;
      }

      if (!beet_mir_lower_expr_into_local(ctx, field_name, strlen(field_name),
                                          expr->field_inits[i].value)) {
        return 0;
      }
    }

    return 1;
  }

  if (!beet_mir_lower_expr(ctx, expr, &temp)) {
    return 0;
  }

  return beet_mir_add_bind_local(ctx->function, name, name_len, temp);
}

int beet_mir_lower_binding(beet_mir_function *function,
                           const beet_ast_binding *binding) {
  beet_mir_lower_context ctx;

  assert(function != NULL);
  assert(binding != NULL);

  memset(&ctx, 0, sizeof(ctx));
  ctx.function = function;
  return beet_mir_lower_expr_into_local(&ctx, binding->name, binding->name_len,
                                        &binding->expr);
}

static int beet_mir_register_binding_type(beet_mir_lower_context *ctx,
                                          const beet_ast_binding *binding) {
  const char *type_name;
  size_t type_name_len;
  const beet_ast_type_decl *type_decl;

  assert(ctx != NULL);
  assert(binding != NULL);

  type_decl = binding->resolved_type_decl;
  if (binding->has_type) {
    type_name = binding->type_name;
    type_name_len = binding->type_name_len;
  } else if (binding->expr.resolved_type_decl != NULL) {
    type_name = binding->expr.resolved_type_decl->name;
    type_name_len = binding->expr.resolved_type_decl->name_len;
    type_decl = binding->expr.resolved_type_decl;
  } else if (binding->expr.kind == BEET_AST_EXPR_INT_LITERAL) {
    type_name = "Int";
    type_name_len = 3U;
  } else if (binding->expr.kind == BEET_AST_EXPR_BOOL_LITERAL) {
    type_name = "Bool";
    type_name_len = 4U;
  } else {
    return 1;
  }

  return beet_mir_context_bind_local_type(ctx, binding->name, binding->name_len,
                                          type_name, type_name_len, type_decl);
}

static int
beet_mir_register_param_locals(beet_mir_lower_context *ctx,
                               const beet_ast_function *function_ast) {
  size_t i;

  assert(ctx != NULL);
  assert(function_ast != NULL);

  ctx->function->param_count = function_ast->param_count;

  for (i = 0U; i < function_ast->param_count; ++i) {
    if (ctx->function->local_count >= BEET_MIR_MAX_LOCALS) {
      return 0;
    }

    beet_copy_name(ctx->function->locals[ctx->function->local_count],
                   function_ast->params[i].name,
                   function_ast->params[i].name_len);
    ctx->function->local_count += 1U;

    if (!beet_mir_context_bind_local_type(
            ctx, function_ast->params[i].name, function_ast->params[i].name_len,
            function_ast->params[i].type_name,
            function_ast->params[i].type_name_len,
            function_ast->params[i].resolved_type_decl)) {
      return 0;
    }
  }

  return 1;
}

static int beet_mir_lower_expr(beet_mir_lower_context *ctx,
                               const beet_ast_expr *expr, int *out_temp) {
  char local_name[BEET_MIR_MAX_NAME_LEN];
  const beet_ast_expr *value_expr;
  size_t local_name_len;
  int lhs_temp;
  int rhs_temp;
  int arg_temps[BEET_AST_MAX_PARAMS];
  size_t i;

  assert(ctx != NULL);
  assert(expr != NULL);
  assert(out_temp != NULL);

  switch (expr->kind) {
  case BEET_AST_EXPR_INT_LITERAL:
    if (!beet_parse_int_slice(expr->text, expr->text_len, &lhs_temp)) {
      return 0;
    }

    *out_temp = beet_mir_add_const_int(ctx->function, lhs_temp);
    return *out_temp >= 0;

  case BEET_AST_EXPR_BOOL_LITERAL:
    if (expr->text_len == 4U) {
      *out_temp = beet_mir_add_const_int(ctx->function, 1);
      return *out_temp >= 0;
    }
    if (expr->text_len == 5U) {
      *out_temp = beet_mir_add_const_int(ctx->function, 0);
      return *out_temp >= 0;
    }
    return 0;

  case BEET_AST_EXPR_NAME:
    *out_temp =
        beet_mir_add_load_local(ctx->function, expr->text, expr->text_len);
    return *out_temp >= 0;

  case BEET_AST_EXPR_CALL:
    if (expr->arg_count > BEET_AST_MAX_PARAMS) {
      return 0;
    }

    for (i = 0U; i < expr->arg_count; ++i) {
      if (expr->args[i] == NULL ||
          !beet_mir_lower_expr(ctx, expr->args[i], &arg_temps[i])) {
        return 0;
      }
    }

    *out_temp = beet_mir_add_call(ctx->function, expr->text, expr->text_len,
                                  arg_temps, expr->arg_count);
    return *out_temp >= 0;

  case BEET_AST_EXPR_FIELD:
    value_expr = beet_mir_resolve_field_value_expr(expr);
    if (value_expr != NULL) {
      return beet_mir_lower_expr(ctx, value_expr, out_temp);
    }

    local_name[0] = '\0';
    local_name_len = 0U;
    if (!beet_mir_build_field_path(expr, local_name, &local_name_len)) {
      return 0;
    }

    *out_temp =
        beet_mir_add_load_local(ctx->function, local_name, local_name_len);
    return *out_temp >= 0;

  case BEET_AST_EXPR_UNARY: {
    int zero_temp;

    if (expr->unary_op != BEET_AST_UNARY_NEGATE || expr->left == NULL) {
      return 0;
    }

    zero_temp = beet_mir_add_const_int(ctx->function, 0);
    if (zero_temp < 0) {
      return 0;
    }

    if (!beet_mir_lower_expr(ctx, expr->left, &rhs_temp)) {
      return 0;
    }

    *out_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_SUB_INT,
                                        zero_temp, rhs_temp);
    return *out_temp >= 0;
  }

  case BEET_AST_EXPR_BINARY:
    if (expr->left == NULL || expr->right == NULL) {
      return 0;
    }

    if (!beet_mir_lower_expr(ctx, expr->left, &lhs_temp)) {
      return 0;
    }

    if (!beet_mir_lower_expr(ctx, expr->right, &rhs_temp)) {
      return 0;
    }

    switch (expr->binary_op) {
    case BEET_AST_BINARY_ADD:
      *out_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_ADD_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_SUB:
      *out_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_SUB_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_MUL:
      *out_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_MUL_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_DIV:
      *out_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_DIV_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_EQ:
      *out_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_EQ_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_NE:
      *out_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_NE_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_LT:
      *out_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_LT_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_LE:
      *out_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_LE_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_GT:
      *out_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_GT_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    case BEET_AST_BINARY_GE:
      *out_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_GE_INT,
                                          lhs_temp, rhs_temp);
      return *out_temp >= 0;

    default:
      return 0;
    }

  default:
    return 0;
  }
}

typedef struct beet_mir_match_scrutinee {
  const beet_ast_type_decl *type_decl;
  const beet_ast_expr *construct_expr;
  char local_name[BEET_MIR_MAX_NAME_LEN];
  size_t local_name_len;
  int is_construct;
} beet_mir_match_scrutinee;

static int beet_mir_get_match_scrutinee(beet_mir_lower_context *ctx,
                                        const beet_ast_expr *expr,
                                        beet_mir_match_scrutinee *out) {
  const beet_mir_local_type *local_type;

  assert(ctx != NULL);
  assert(expr != NULL);
  assert(out != NULL);

  memset(out, 0, sizeof(*out));

  if (expr->kind == BEET_AST_EXPR_CONSTRUCT) {
    out->type_decl = expr->resolved_type_decl;
    if (out->type_decl == NULL || !out->type_decl->is_choice) {
      return 0;
    }
    out->construct_expr = expr;
    out->is_construct = 1;
    return 1;
  }

  if (expr->kind != BEET_AST_EXPR_NAME) {
    return 0;
  }

  local_type =
      beet_mir_context_find_local_type(ctx, expr->text, expr->text_len);
  if (local_type == NULL) {
    return 0;
  }

  out->type_decl = local_type->type_decl;
  if (out->type_decl == NULL || !out->type_decl->is_choice) {
    return 0;
  }

  beet_copy_name(out->local_name, expr->text, expr->text_len);
  out->local_name_len = strlen(out->local_name);
  return 1;
}

static int beet_mir_lower_return_stmt(beet_mir_lower_context *ctx,
                                      const beet_ast_stmt *stmt) {
  int value;
  int temp;

  assert(ctx != NULL);
  assert(stmt != NULL);
  assert(stmt->kind == BEET_AST_STMT_RETURN);

  switch (stmt->expr.kind) {
  case BEET_AST_EXPR_INT_LITERAL:
    if (!beet_parse_int_slice(stmt->expr.text, stmt->expr.text_len, &value)) {
      return 0;
    }
    return beet_mir_add_return_const_int(ctx->function, value);

  case BEET_AST_EXPR_BOOL_LITERAL:
    if (stmt->expr.text_len == 4U) {
      return beet_mir_add_return_const_int(ctx->function, 1);
    }
    if (stmt->expr.text_len == 5U) {
      return beet_mir_add_return_const_int(ctx->function, 0);
    }
    return 0;

  case BEET_AST_EXPR_NAME:
    return beet_mir_add_return_local(ctx->function, stmt->expr.text,
                                     stmt->expr.text_len);

  default:
    if (!beet_mir_lower_expr(ctx, &stmt->expr, &temp)) {
      return 0;
    }
    return beet_mir_add_return_temp(ctx->function, temp);
  }
}

static int beet_mir_lower_stmt_list(beet_mir_lower_context *ctx,
                                    const beet_ast_stmt *stmts,
                                    size_t stmt_count);

static int beet_mir_lower_match_stmt(beet_mir_lower_context *ctx,
                                     const beet_ast_stmt *stmt) {
  beet_mir_match_scrutinee scrutinee;
  int tag_temp;
  int end_label;
  size_t case_index;

  assert(ctx != NULL);
  assert(stmt != NULL);
  assert(stmt->kind == BEET_AST_STMT_MATCH);

  if (!beet_mir_get_match_scrutinee(ctx, &stmt->match_expr, &scrutinee)) {
    return 0;
  }

  if (scrutinee.is_construct) {
    if (scrutinee.construct_expr->resolved_variant_decl == NULL) {
      return 0;
    }
    tag_temp = beet_mir_add_const_int(
        ctx->function, (int)scrutinee.construct_expr->resolved_variant_index);
  } else {
    char tag_name[BEET_MIR_MAX_NAME_LEN];

    if (!beet_mir_build_choice_tag_name(tag_name, scrutinee.local_name,
                                        scrutinee.local_name_len)) {
      return 0;
    }
    tag_temp =
        beet_mir_add_load_local(ctx->function, tag_name, strlen(tag_name));
  }

  if (tag_temp < 0) {
    return 0;
  }

  end_label = beet_mir_next_label(ctx->function);

  for (case_index = 0U; case_index < stmt->match_case_count; ++case_index) {
    const beet_ast_match_case *match_case = &stmt->match_cases[case_index];
    const beet_ast_choice_variant *variant_decl;
    size_t variant_index;
    size_t local_count_before;
    int next_label;
    int case_tag_temp;
    int condition_temp;

    variant_decl = match_case->resolved_variant_decl;
    variant_index = match_case->resolved_variant_index;
    if (variant_decl == NULL) {
      return 0;
    }

    case_tag_temp = beet_mir_add_const_int(ctx->function, (int)variant_index);
    if (case_tag_temp < 0) {
      return 0;
    }
    condition_temp = beet_mir_add_binary_int(ctx->function, BEET_MIR_OP_EQ_INT,
                                             tag_temp, case_tag_temp);
    if (condition_temp < 0) {
      return 0;
    }

    next_label = beet_mir_next_label(ctx->function);
    if (!beet_mir_add_jump_if_false(ctx->function, condition_temp,
                                    next_label)) {
      return 0;
    }

    local_count_before = ctx->local_count;

    if (match_case->binds_payload) {
      if (!variant_decl->has_payload) {
        return 0;
      }

      if (scrutinee.is_construct) {
        if (scrutinee.construct_expr->field_inits[0].value == NULL ||
            !beet_mir_lower_expr_into_local(
                ctx, match_case->binding_name, match_case->binding_name_len,
                scrutinee.construct_expr->field_inits[0].value)) {
          return 0;
        }
      } else {
        char payload_name[BEET_MIR_MAX_NAME_LEN];

        if (!beet_mir_build_choice_variant_storage_name(
                payload_name, scrutinee.local_name, scrutinee.local_name_len,
                match_case->variant_name, match_case->variant_name_len)) {
          return 0;
        }
        if (!beet_mir_copy_local_value(
                ctx, match_case->binding_name, match_case->binding_name_len,
                payload_name, strlen(payload_name),
                variant_decl->payload_type_name,
                variant_decl->payload_type_name_len, NULL)) {
          return 0;
        }
      }

      if (!beet_mir_context_bind_local_type(
              ctx, match_case->binding_name, match_case->binding_name_len,
              variant_decl->payload_type_name,
              variant_decl->payload_type_name_len, NULL)) {
        return 0;
      }
    }

    if (!beet_mir_lower_stmt_list(ctx, match_case->body,
                                  match_case->body_count)) {
      return 0;
    }
    ctx->local_count = local_count_before;

    if (!beet_mir_add_jump(ctx->function, end_label) ||
        !beet_mir_add_label(ctx->function, next_label)) {
      return 0;
    }
  }

  return beet_mir_add_label(ctx->function, end_label);
}

static int beet_mir_lower_while_stmt(beet_mir_lower_context *ctx,
                                     const beet_ast_stmt *stmt) {
  int start_label;
  int end_label;
  int condition_temp;
  size_t local_count_before;

  assert(ctx != NULL);
  assert(stmt != NULL);
  assert(stmt->kind == BEET_AST_STMT_WHILE);

  start_label = beet_mir_next_label(ctx->function);
  end_label = beet_mir_next_label(ctx->function);

  if (!beet_mir_add_label(ctx->function, start_label)) {
    return 0;
  }

  if (!beet_mir_lower_expr(ctx, &stmt->condition, &condition_temp)) {
    return 0;
  }

  if (!beet_mir_add_jump_if_false(ctx->function, condition_temp, end_label)) {
    return 0;
  }

  local_count_before = ctx->local_count;
  if (!beet_mir_lower_stmt_list(ctx, stmt->loop_body, stmt->loop_body_count)) {
    return 0;
  }
  ctx->local_count = local_count_before;

  if (!beet_mir_add_jump(ctx->function, start_label)) {
    return 0;
  }

  return beet_mir_add_label(ctx->function, end_label);
}

static int beet_mir_lower_if_stmt(beet_mir_lower_context *ctx,
                                  const beet_ast_stmt *stmt) {
  int condition_temp;
  int else_label;
  int end_label;
  size_t local_count_before;

  assert(ctx != NULL);
  assert(stmt != NULL);
  assert(stmt->kind == BEET_AST_STMT_IF);

  if (!beet_mir_lower_expr(ctx, &stmt->condition, &condition_temp)) {
    return 0;
  }

  else_label = beet_mir_next_label(ctx->function);
  if (!beet_mir_add_jump_if_false(ctx->function, condition_temp, else_label)) {
    return 0;
  }

  local_count_before = ctx->local_count;
  if (!beet_mir_lower_stmt_list(ctx, stmt->then_body, stmt->then_body_count)) {
    return 0;
  }
  ctx->local_count = local_count_before;

  if (stmt->else_body_count == 0U) {
    return beet_mir_add_label(ctx->function, else_label);
  }

  end_label = beet_mir_next_label(ctx->function);
  if (!beet_mir_add_jump(ctx->function, end_label)) {
    return 0;
  }

  if (!beet_mir_add_label(ctx->function, else_label)) {
    return 0;
  }

  local_count_before = ctx->local_count;
  if (!beet_mir_lower_stmt_list(ctx, stmt->else_body, stmt->else_body_count)) {
    return 0;
  }
  ctx->local_count = local_count_before;

  return beet_mir_add_label(ctx->function, end_label);
}

static int beet_mir_lower_stmt_list(beet_mir_lower_context *ctx,
                                    const beet_ast_stmt *stmts,
                                    size_t stmt_count) {
  size_t i;

  assert(ctx != NULL);
  assert(stmts != NULL || stmt_count == 0U);

  for (i = 0U; i < stmt_count; ++i) {
    const beet_ast_stmt *stmt = &stmts[i];

    switch (stmt->kind) {
    case BEET_AST_STMT_BINDING:
      if (!beet_mir_lower_expr_into_local(ctx, stmt->binding.name,
                                          stmt->binding.name_len,
                                          &stmt->binding.expr) ||
          !beet_mir_register_binding_type(ctx, &stmt->binding)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_ASSIGNMENT: {
      int temp;

      if (!beet_mir_lower_expr(ctx, &stmt->expr, &temp)) {
        return 0;
      }

      if (!beet_mir_add_store_local(ctx->function, stmt->assignment.name,
                                    stmt->assignment.name_len, temp)) {
        return 0;
      }
      break;
    }

    case BEET_AST_STMT_RETURN:
      if (!beet_mir_lower_return_stmt(ctx, stmt)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_IF:
      if (!beet_mir_lower_if_stmt(ctx, stmt)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_WHILE:
      if (!beet_mir_lower_while_stmt(ctx, stmt)) {
        return 0;
      }
      break;

    case BEET_AST_STMT_MATCH:
      if (!beet_mir_lower_match_stmt(ctx, stmt)) {
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

int beet_mir_lower_function_with_type_decls(
    beet_mir_function *function, const beet_ast_function *function_ast,
    const beet_ast_type_decl *type_decls, size_t decl_count) {
  beet_mir_lower_context ctx;

  assert(function != NULL);
  assert(function_ast != NULL);
  assert(type_decls != NULL || decl_count == 0U);

  memset(&ctx, 0, sizeof(ctx));
  ctx.function = function;
  ctx.type_decls = type_decls;
  ctx.decl_count = decl_count;

  beet_mir_function_init(function, function_ast->name, function_ast->name_len);
  if (!beet_mir_register_param_locals(&ctx, function_ast)) {
    return 0;
  }

  return beet_mir_lower_stmt_list(&ctx, function_ast->body,
                                  function_ast->body_count);
}

int beet_mir_lower_function_with_registry(beet_mir_function *function,
                                          const beet_ast_function *function_ast,
                                          const beet_decl_registry *registry) {
  assert(registry != NULL);

  return beet_mir_lower_function_with_type_decls(
      function, function_ast, registry->type_decls, registry->type_decl_count);
}

int beet_mir_lower_function(beet_mir_function *function,
                            const beet_ast_function *function_ast) {
  return beet_mir_lower_function_with_type_decls(function, function_ast, NULL,
                                                 0U);
}
