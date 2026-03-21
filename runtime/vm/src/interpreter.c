#include "beet/vm/interpreter.h"

#include <assert.h>
#include <string.h>

static size_t beet_vm_instr_width(int op) {
  switch (op) {
  case BEET_BC_OP_CONST_INT:
  case BEET_BC_OP_STORE_LOCAL:
  case BEET_BC_OP_LOAD_LOCAL:
  case BEET_BC_OP_JUMP_IF_FALSE:
    return 3U;

  case BEET_BC_OP_ADD_INT:
  case BEET_BC_OP_SUB_INT:
  case BEET_BC_OP_MUL_INT:
  case BEET_BC_OP_DIV_INT:
    return 4U;

  case BEET_BC_OP_RETURN_LOCAL:
  case BEET_BC_OP_RETURN_TEMP:
  case BEET_BC_OP_RETURN_CONST_INT:
  case BEET_BC_OP_LABEL:
  case BEET_BC_OP_JUMP:
    return 2U;

  default:
    return 0U;
  }
}

static int beet_vm_find_label_pc(const beet_bytecode_function *function,
                                 int label_id, size_t *out_pc) {
  size_t pc;

  assert(function != NULL);
  assert(out_pc != NULL);

  pc = 0U;
  while (pc < function->code_count) {
    int op = function->code[pc];
    size_t width = beet_vm_instr_width(op);

    if (width == 0U || pc + width > function->code_count) {
      return 0;
    }

    if (op == BEET_BC_OP_LABEL && function->code[pc + 1U] == label_id) {
      *out_pc = pc + width;
      return 1;
    }

    pc += width;
  }

  return 0;
}

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

    case BEET_BC_OP_LABEL:
      pc += 1U;
      break;

    case BEET_BC_OP_JUMP: {
      int label_id = function->code[pc++];
      if (!beet_vm_find_label_pc(function, label_id, &pc)) {
        return 0;
      }
      break;
    }

    case BEET_BC_OP_JUMP_IF_FALSE: {
      int condition_temp = function->code[pc++];
      int label_id = function->code[pc++];
      if (vm->regs[condition_temp] == 0) {
        if (!beet_vm_find_label_pc(function, label_id, &pc)) {
          return 0;
        }
      }
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
