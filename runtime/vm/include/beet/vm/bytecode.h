#ifndef BEET_VM_BYTECODE_H
#define BEET_VM_BYTECODE_H

#include <stddef.h>

#define BEET_BC_MAX_CODE 512

typedef enum beet_bc_opcode {
    BEET_BC_OP_CONST_INT = 1,
    BEET_BC_OP_STORE_LOCAL = 2,
    BEET_BC_OP_RETURN_LOCAL = 3,
    BEET_BC_OP_RETURN_CONST_INT = 4
} beet_bc_opcode;

typedef struct beet_bytecode_function {
    int code[BEET_BC_MAX_CODE];
    size_t code_count;
    size_t local_count;
} beet_bytecode_function;

void beet_bytecode_function_init(beet_bytecode_function *function);

int beet_bytecode_emit_const_int(
    beet_bytecode_function *function,
    int dst,
    int value
);

int beet_bytecode_emit_store_local(
    beet_bytecode_function *function,
    int local_index,
    int src_temp
);

int beet_bytecode_emit_return_local(
    beet_bytecode_function *function,
    int local_index
);

int beet_bytecode_emit_return_const_int(
    beet_bytecode_function *function,
    int value
);

#endif