#ifndef BEET_VM_INTERPRETER_H
#define BEET_VM_INTERPRETER_H

#include "beet/vm/bytecode.h"

#define BEET_VM_MAX_REGS 256
#define BEET_VM_MAX_LOCALS 128

typedef struct beet_vm {
    int regs[BEET_VM_MAX_REGS];
    int locals[BEET_VM_MAX_LOCALS];
} beet_vm;

/* returns 1 on success, and sets *out_result */
int beet_vm_execute(
    beet_vm *vm,
    const beet_bytecode_function *function,
    int *out_result
);

#endif