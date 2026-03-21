#ifndef BEET_VM_BYTECODE_H
#define BEET_VM_BYTECODE_H

#include <stddef.h>

#define BEET_BC_MAX_CODE 512

typedef enum beet_bc_opcode {
  BEET_BC_OP_CONST_INT = 1,
  BEET_BC_OP_STORE_LOCAL = 2,
  BEET_BC_OP_LOAD_LOCAL = 3,
  BEET_BC_OP_ADD_INT = 4,
  BEET_BC_OP_SUB_INT = 5,
  BEET_BC_OP_MUL_INT = 6,
  BEET_BC_OP_DIV_INT = 7,
  BEET_BC_OP_EQ_INT = 8,
  BEET_BC_OP_NE_INT = 9,
  BEET_BC_OP_LT_INT = 10,
  BEET_BC_OP_LE_INT = 11,
  BEET_BC_OP_GT_INT = 12,
  BEET_BC_OP_GE_INT = 13,
  BEET_BC_OP_RETURN_LOCAL = 14,
  BEET_BC_OP_RETURN_TEMP = 15,
  BEET_BC_OP_RETURN_CONST_INT = 16,
  BEET_BC_OP_LABEL = 17,
  BEET_BC_OP_JUMP = 18,
  BEET_BC_OP_JUMP_IF_FALSE = 19
} beet_bc_opcode;

typedef struct beet_bytecode_function {
  int code[BEET_BC_MAX_CODE];
  size_t code_count;
  size_t local_count;
} beet_bytecode_function;

void beet_bytecode_function_init(beet_bytecode_function *function);

int beet_bytecode_emit2(beet_bytecode_function *function, int a, int b);
int beet_bytecode_emit3(beet_bytecode_function *function, int a, int b, int c);
int beet_bytecode_emit4(beet_bytecode_function *function, int a, int b, int c,
                        int d);

int beet_bytecode_emit_store_local(beet_bytecode_function *function,
                                   int local_index, int src_temp);

int beet_bytecode_emit_binary_int(beet_bytecode_function *function,
                                  beet_bc_opcode opcode, int dst_temp,
                                  int lhs_temp, int rhs_temp);

#endif
