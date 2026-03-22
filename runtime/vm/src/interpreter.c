#include "beet/vm/interpreter.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static beet_vm_frame *beet_vm_current_frame(beet_vm *vm) {
  assert(vm != NULL);
  assert(vm->frame_count > 0U);

  return &vm->frames[vm->frame_count - 1U];
}

static beet_bytecode_function *
beet_vm_current_function(const beet_vm *vm, beet_bytecode_program *program) {
  const beet_vm_frame *frame;

  assert(vm != NULL);
  assert(program != NULL);
  assert(vm->frame_count > 0U);

  frame = &vm->frames[vm->frame_count - 1U];
  if (frame->function_index >= program->function_count) {
    return NULL;
  }

  return &program->functions[frame->function_index];
}

static int beet_vm_push_frame(beet_vm *vm, size_t function_index,
                              int return_dest) {
  beet_vm_frame *frame;

  assert(vm != NULL);

  if (vm->frame_count >= BEET_VM_MAX_CALL_FRAMES) {
    return 0;
  }

  frame = &vm->frames[vm->frame_count];
  memset(frame, 0, sizeof(*frame));
  frame->function_index = function_index;
  frame->pc = 0U;
  frame->return_dest = return_dest;
  vm->frame_count += 1U;
  return 1;
}

int beet_vm_execute_program(beet_vm *vm, beet_bytecode_program *program,
                            size_t entry_index, int *out_result) {
  size_t i;

  assert(vm != NULL);
  assert(program != NULL);
  assert(out_result != NULL);

  if (entry_index >= program->function_count) {
    return 0;
  }

  for (i = 0U; i < program->function_count; ++i) {
    if (!beet_bytecode_prepare_jump_targets(&program->functions[i])) {
      return 0;
    }
  }

  vm->frame_count = 0U;
  if (!beet_vm_push_frame(vm, entry_index, -1)) {
    return 0;
  }

  while (vm->frame_count > 0U) {
    beet_vm_frame *frame = beet_vm_current_frame(vm);
    beet_bytecode_function *function = beet_vm_current_function(vm, program);

    if (function == NULL || frame->pc >= function->code_count) {
      return 0;
    }

    switch (function->code[frame->pc]) {
    case BEET_BC_OP_CONST_INT: {
      int dst = function->code[frame->pc + 1U];
      int value = function->code[frame->pc + 2U];
      frame->regs[dst] = value;
      frame->pc += 3U;
      break;
    }

    case BEET_BC_OP_STORE_LOCAL: {
      int local_index = function->code[frame->pc + 1U];
      int src = function->code[frame->pc + 2U];
      frame->locals[local_index] = frame->regs[src];
      frame->pc += 3U;
      break;
    }

    case BEET_BC_OP_LOAD_LOCAL: {
      int dst = function->code[frame->pc + 1U];
      int local_index = function->code[frame->pc + 2U];
      frame->regs[dst] = frame->locals[local_index];
      frame->pc += 3U;
      break;
    }

    case BEET_BC_OP_CALL: {
      const beet_bytecode_function *callee;
      int dst = function->code[frame->pc + 1U];
      int callee_index = function->code[frame->pc + 2U];
      size_t arg_count = (size_t)function->code[frame->pc + 3U];
      size_t arg_index;
      beet_vm_frame *callee_frame;

      if (callee_index < 0 || (size_t)callee_index >= program->function_count) {
        return 0;
      }

      callee = &program->functions[callee_index];
      if (arg_count != callee->param_count || arg_count > BEET_VM_MAX_LOCALS) {
        return 0;
      }

      frame->pc += 4U + arg_count;
      if (!beet_vm_push_frame(vm, (size_t)callee_index, dst)) {
        return 0;
      }

      callee_frame = beet_vm_current_frame(vm);
      for (arg_index = 0U; arg_index < arg_count; ++arg_index) {
        int arg_temp = function->code[frame->pc - arg_count + arg_index];
        callee_frame->locals[arg_index] = frame->regs[arg_temp];
      }
      break;
    }

    case BEET_BC_OP_ADD_INT: {
      int dst = function->code[frame->pc + 1U];
      int lhs = function->code[frame->pc + 2U];
      int rhs = function->code[frame->pc + 3U];
      frame->regs[dst] = frame->regs[lhs] + frame->regs[rhs];
      frame->pc += 4U;
      break;
    }

    case BEET_BC_OP_SUB_INT: {
      int dst = function->code[frame->pc + 1U];
      int lhs = function->code[frame->pc + 2U];
      int rhs = function->code[frame->pc + 3U];
      frame->regs[dst] = frame->regs[lhs] - frame->regs[rhs];
      frame->pc += 4U;
      break;
    }

    case BEET_BC_OP_MUL_INT: {
      int dst = function->code[frame->pc + 1U];
      int lhs = function->code[frame->pc + 2U];
      int rhs = function->code[frame->pc + 3U];
      frame->regs[dst] = frame->regs[lhs] * frame->regs[rhs];
      frame->pc += 4U;
      break;
    }

    case BEET_BC_OP_DIV_INT: {
      int dst = function->code[frame->pc + 1U];
      int lhs = function->code[frame->pc + 2U];
      int rhs = function->code[frame->pc + 3U];
      if (frame->regs[rhs] == 0) {
        return 0;
      }
      frame->regs[dst] = frame->regs[lhs] / frame->regs[rhs];
      frame->pc += 4U;
      break;
    }

    case BEET_BC_OP_EQ_INT: {
      int dst = function->code[frame->pc + 1U];
      int lhs = function->code[frame->pc + 2U];
      int rhs = function->code[frame->pc + 3U];
      frame->regs[dst] = (frame->regs[lhs] == frame->regs[rhs]) ? 1 : 0;
      frame->pc += 4U;
      break;
    }

    case BEET_BC_OP_NE_INT: {
      int dst = function->code[frame->pc + 1U];
      int lhs = function->code[frame->pc + 2U];
      int rhs = function->code[frame->pc + 3U];
      frame->regs[dst] = (frame->regs[lhs] != frame->regs[rhs]) ? 1 : 0;
      frame->pc += 4U;
      break;
    }

    case BEET_BC_OP_LT_INT: {
      int dst = function->code[frame->pc + 1U];
      int lhs = function->code[frame->pc + 2U];
      int rhs = function->code[frame->pc + 3U];
      frame->regs[dst] = (frame->regs[lhs] < frame->regs[rhs]) ? 1 : 0;
      frame->pc += 4U;
      break;
    }

    case BEET_BC_OP_LE_INT: {
      int dst = function->code[frame->pc + 1U];
      int lhs = function->code[frame->pc + 2U];
      int rhs = function->code[frame->pc + 3U];
      frame->regs[dst] = (frame->regs[lhs] <= frame->regs[rhs]) ? 1 : 0;
      frame->pc += 4U;
      break;
    }

    case BEET_BC_OP_GT_INT: {
      int dst = function->code[frame->pc + 1U];
      int lhs = function->code[frame->pc + 2U];
      int rhs = function->code[frame->pc + 3U];
      frame->regs[dst] = (frame->regs[lhs] > frame->regs[rhs]) ? 1 : 0;
      frame->pc += 4U;
      break;
    }

    case BEET_BC_OP_GE_INT: {
      int dst = function->code[frame->pc + 1U];
      int lhs = function->code[frame->pc + 2U];
      int rhs = function->code[frame->pc + 3U];
      frame->regs[dst] = (frame->regs[lhs] >= frame->regs[rhs]) ? 1 : 0;
      frame->pc += 4U;
      break;
    }

    case BEET_BC_OP_LABEL:
      frame->pc += 2U;
      break;

    case BEET_BC_OP_JUMP: {
      int label_id = function->code[frame->pc + 1U];
      if (label_id < 0 || (size_t)label_id >= function->jump_target_count ||
          function->jump_target_pcs[label_id] == SIZE_MAX) {
        return 0;
      }
      frame->pc = function->jump_target_pcs[label_id];
      break;
    }

    case BEET_BC_OP_JUMP_IF_FALSE: {
      int condition_temp = function->code[frame->pc + 1U];
      int label_id = function->code[frame->pc + 2U];
      if (frame->regs[condition_temp] == 0) {
        if (label_id < 0 || (size_t)label_id >= function->jump_target_count ||
            function->jump_target_pcs[label_id] == SIZE_MAX) {
          return 0;
        }
        frame->pc = function->jump_target_pcs[label_id];
      } else {
        frame->pc += 3U;
      }
      break;
    }

    case BEET_BC_OP_RETURN_LOCAL:
    case BEET_BC_OP_RETURN_TEMP:
    case BEET_BC_OP_RETURN_CONST_INT: {
      beet_vm_frame returning_frame;
      int value;

      if (function->code[frame->pc] == BEET_BC_OP_RETURN_LOCAL) {
        value = frame->locals[function->code[frame->pc + 1U]];
      } else if (function->code[frame->pc] == BEET_BC_OP_RETURN_TEMP) {
        value = frame->regs[function->code[frame->pc + 1U]];
      } else {
        value = function->code[frame->pc + 1U];
      }

      returning_frame = *frame;
      vm->frame_count -= 1U;

      if (vm->frame_count == 0U) {
        *out_result = value;
        return 1;
      }

      frame = beet_vm_current_frame(vm);
      frame->regs[returning_frame.return_dest] = value;
      break;
    }

    default:
      return 0;
    }
  }

  return 0;
}

int beet_vm_execute(beet_vm *vm, beet_bytecode_function *function,
                    int *out_result) {
  beet_bytecode_program program;

  assert(vm != NULL);
  assert(function != NULL);
  assert(out_result != NULL);

  program.functions = function;
  program.function_count = 1U;
  return beet_vm_execute_program(vm, &program, 0U, out_result);
}
