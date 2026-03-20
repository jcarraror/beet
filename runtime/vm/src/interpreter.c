#include "beet/vm/interpreter.h"

#include <assert.h>

int beet_vm_execute(beet_vm *vm, const beet_bytecode_function *function,
                    int *out_result) {
  size_t pc;

  assert(vm != NULL);
  assert(function != NULL);
  assert(out_result != NULL);

  pc = 0U;

  while (pc < function->code_count) {
    int op = function->code[pc++];

    switch (op) {
    case BEET_BC_OP_CONST_INT: {
      int dst = function->code[pc++];
      int value = function->code[pc++];
      vm->regs[dst] = value;
      break;
    }

    case BEET_BC_OP_STORE_LOCAL: {
      int local_index = function->code[pc++];
      int src = function->code[pc++];
      vm->locals[local_index] = vm->regs[src];
      break;
    }

    case BEET_BC_OP_RETURN_LOCAL: {
      int local_index = function->code[pc++];
      *out_result = vm->locals[local_index];
      return 1;
    }

    case BEET_BC_OP_RETURN_CONST_INT: {
      int value = function->code[pc++];
      *out_result = value;
      return 1;
    }

    default:
      return 0;
    }
  }

  return 0;
}