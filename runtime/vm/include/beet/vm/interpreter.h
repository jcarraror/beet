#ifndef BEET_VM_INTERPRETER_H
#define BEET_VM_INTERPRETER_H

#include "beet/vm/bytecode.h"

#define BEET_VM_MAX_REGS 256
#define BEET_VM_MAX_LOCALS 128
#define BEET_VM_MAX_CALL_FRAMES 32

typedef struct beet_vm_frame {
  int regs[BEET_VM_MAX_REGS];
  int locals[BEET_VM_MAX_LOCALS];
  size_t function_index;
  size_t pc;
  int return_dest;
} beet_vm_frame;

typedef struct beet_vm {
  beet_vm_frame frames[BEET_VM_MAX_CALL_FRAMES];
  size_t frame_count;
} beet_vm;

/* returns 1 on success, and sets *out_result */
int beet_vm_execute(beet_vm *vm, const beet_bytecode_function *function,
                    int *out_result);
int beet_vm_execute_program(beet_vm *vm, const beet_bytecode_program *program,
                            size_t entry_index, int *out_result);

#endif
