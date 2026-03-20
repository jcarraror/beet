#include "beet/vm/interpreter.h"

#include <assert.h>
#include <string.h>

int beet_vm_execute(beet_vm *vm, const beet_bytecode_function *function,
                    int *out_result) {
  size_t pc;

  assert(vm != NULL);
  assert(function != NULL);
  assert(out_result != NULL);

  memset(vm->regs, 0, sizeof(vm->regs));
  memset(vm->locals, 0, sizeof(vm->locals));

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

    case BEET_BC_OP_LOAD_LOCAL: {
      int dst = function->code[pc++];
      int local_index = function->code[pc++];
      vm->regs[dst] = vm->locals[local_index];
      break;
    }

    case BEET_BC_OP_ADD_INT: {
      int dst = function->code[pc++];
      int lhs = function->code[pc++];
      int rhs = function->code[pc++];
      vm->regs[dst] = vm->regs[lhs] + vm->regs[rhs];
      break;
    }

    case BEET_BC_OP_SUB_INT: {
      int dst = function->code[pc++];
      int lhs = function->code[pc++];
      int rhs = function->code[pc++];
      vm->regs[dst] = vm->regs[lhs] - vm->regs[rhs];
      break;
    }

    case BEET_BC_OP_MUL_INT: {
      int dst = function->code[pc++];
      int lhs = function->code[pc++];
      int rhs = function->code[pc++];
      vm->regs[dst] = vm->regs[lhs] * vm->regs[rhs];
      break;
    }

    case BEET_BC_OP_DIV_INT: {
      int dst = function->code[pc++];
      int lhs = function->code[pc++];
      int rhs = function->code[pc++];
      if (vm->regs[rhs] == 0) {
        return 0;
      }
      vm->regs[dst] = vm->regs[lhs] / vm->regs[rhs];
      break;
    }

    case BEET_BC_OP_RETURN_LOCAL: {
      int local_index = function->code[pc++];
      *out_result = vm->locals[local_index];
      return 1;
    }

    case BEET_BC_OP_RETURN_TEMP: {
      int temp = function->code[pc++];
      *out_result = vm->regs[temp];
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
