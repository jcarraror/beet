#ifndef BEET_MIR_MIR_H
#define BEET_MIR_MIR_H

#include <stddef.h>

#include "beet/parser/ast.h"

#define BEET_MIR_MAX_INSTRS 256
#define BEET_MIR_MAX_LOCALS 128
#define BEET_MIR_MAX_NAME_LEN 64
#define BEET_MIR_MAX_FUNCTIONS 32

typedef enum beet_mir_opcode {
  BEET_MIR_OP_CONST_INT = 0,
  BEET_MIR_OP_BIND_LOCAL,
  BEET_MIR_OP_LOAD_LOCAL,
  BEET_MIR_OP_STORE_LOCAL,
  BEET_MIR_OP_CALL,
  BEET_MIR_OP_ADD_INT,
  BEET_MIR_OP_SUB_INT,
  BEET_MIR_OP_MUL_INT,
  BEET_MIR_OP_DIV_INT,
  BEET_MIR_OP_EQ_INT,
  BEET_MIR_OP_NE_INT,
  BEET_MIR_OP_LT_INT,
  BEET_MIR_OP_LE_INT,
  BEET_MIR_OP_GT_INT,
  BEET_MIR_OP_GE_INT,
  BEET_MIR_OP_LABEL,
  BEET_MIR_OP_JUMP,
  BEET_MIR_OP_JUMP_IF_FALSE,
  BEET_MIR_OP_RETURN_LOCAL,
  BEET_MIR_OP_RETURN_TEMP,
  BEET_MIR_OP_RETURN_CONST_INT
} beet_mir_opcode;

typedef struct beet_mir_instr {
  beet_mir_opcode op;
  int dst;
  int src_lhs;
  int src_rhs;
  int int_value;
  char name[BEET_MIR_MAX_NAME_LEN];
  int args[BEET_AST_MAX_PARAMS];
  size_t arg_count;
} beet_mir_instr;

typedef struct beet_mir_function {
  char name[BEET_MIR_MAX_NAME_LEN];
  beet_mir_instr instrs[BEET_MIR_MAX_INSTRS];
  size_t instr_count;

  char locals[BEET_MIR_MAX_LOCALS][BEET_MIR_MAX_NAME_LEN];
  size_t local_count;
  size_t param_count;

  int next_temp;
  int next_label;
} beet_mir_function;

void beet_mir_function_init(beet_mir_function *function, const char *name,
                            size_t name_len);

int beet_mir_add_const_int(beet_mir_function *function, int value);
int beet_mir_add_bind_local(beet_mir_function *function, const char *name,
                            size_t name_len, int src_temp);
int beet_mir_add_load_local(beet_mir_function *function, const char *name,
                            size_t name_len);
int beet_mir_add_store_local(beet_mir_function *function, const char *name,
                             size_t name_len, int src_temp);
int beet_mir_add_call(beet_mir_function *function, const char *name,
                      size_t name_len, const int *args, size_t arg_count);
int beet_mir_add_binary_int(beet_mir_function *function, beet_mir_opcode op,
                            int lhs_temp, int rhs_temp);
int beet_mir_next_label(beet_mir_function *function);
int beet_mir_add_label(beet_mir_function *function, int label_id);
int beet_mir_add_jump(beet_mir_function *function, int label_id);
int beet_mir_add_jump_if_false(beet_mir_function *function, int condition_temp,
                               int label_id);
int beet_mir_add_return_local(beet_mir_function *function, const char *name,
                              size_t name_len);
int beet_mir_add_return_temp(beet_mir_function *function, int temp);
int beet_mir_add_return_const_int(beet_mir_function *function, int value);
int beet_mir_lower_binding(beet_mir_function *function,
                           const beet_ast_binding *binding);

int beet_mir_lower_trivial_return_function(
    beet_mir_function *function, const beet_ast_function *function_ast,
    size_t entry_index);

int beet_mir_lower_function(beet_mir_function *function,
                            const beet_ast_function *function_ast);

#endif
