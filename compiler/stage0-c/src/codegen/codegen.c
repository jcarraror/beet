#include "beet/codegen/codegen.h"

#include <assert.h>
#include <string.h>

static int beet_find_local_index(const beet_mir_function *function,
                                 const char *name) {
  size_t i;

  assert(function != NULL);
  assert(name != NULL);

  for (i = 0U; i < function->local_count; ++i) {
    if (strcmp(function->locals[i], name) == 0) {
      return (int)i;
    }
  }

  return -1;
}

static int beet_find_function_index(const beet_mir_function *functions,
                                    size_t function_count, const char *name) {
  size_t i;

  assert(functions != NULL);
  assert(name != NULL);

  for (i = 0U; i < function_count; ++i) {
    if (strcmp(functions[i].name, name) == 0) {
      return (int)i;
    }
  }

  return -1;
}

static int
beet_codegen_single_function(const beet_mir_function *mir_function,
                             const beet_mir_function *mir_functions,
                             size_t function_count,
                             beet_bytecode_function *bytecode_function) {
  size_t i;

  assert(mir_function != NULL);
  assert(mir_functions != NULL);
  assert(bytecode_function != NULL);

  beet_bytecode_function_init(bytecode_function);
  bytecode_function->local_count = mir_function->local_count;
  bytecode_function->param_count = mir_function->param_count;

  for (i = 0U; i < mir_function->instr_count; ++i) {
    const beet_mir_instr *instr = &mir_function->instrs[i];

    switch (instr->op) {
    case BEET_MIR_OP_CONST_INT:
      if (!beet_bytecode_emit3(bytecode_function, BEET_BC_OP_CONST_INT,
                               instr->dst, instr->int_value)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_BIND_LOCAL: {
      int local_index = beet_find_local_index(mir_function, instr->name);
      if (local_index < 0) {
        return 0;
      }

      if (!beet_bytecode_emit_store_local(bytecode_function, local_index,
                                          instr->dst)) {
        return 0;
      }
      break;
    }

    case BEET_MIR_OP_LOAD_LOCAL: {
      int local_index = beet_find_local_index(mir_function, instr->name);
      if (local_index < 0) {
        return 0;
      }

      if (!beet_bytecode_emit3(bytecode_function, BEET_BC_OP_LOAD_LOCAL,
                               instr->dst, local_index)) {
        return 0;
      }
      break;
    }

    case BEET_MIR_OP_STORE_LOCAL: {
      int local_index = beet_find_local_index(mir_function, instr->name);
      if (local_index < 0) {
        return 0;
      }

      if (!beet_bytecode_emit_store_local(bytecode_function, local_index,
                                          instr->dst)) {
        return 0;
      }
      break;
    }

    case BEET_MIR_OP_CALL: {
      int function_index =
          beet_find_function_index(mir_functions, function_count, instr->name);
      if (function_index < 0) {
        return 0;
      }

      if (!beet_bytecode_emit_call(bytecode_function, instr->dst,
                                   function_index, instr->args,
                                   instr->arg_count)) {
        return 0;
      }
      break;
    }

    case BEET_MIR_OP_ADD_INT:
      if (!beet_bytecode_emit_binary_int(bytecode_function, BEET_BC_OP_ADD_INT,
                                         instr->dst, instr->src_lhs,
                                         instr->src_rhs)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_SUB_INT:
      if (!beet_bytecode_emit_binary_int(bytecode_function, BEET_BC_OP_SUB_INT,
                                         instr->dst, instr->src_lhs,
                                         instr->src_rhs)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_MUL_INT:
      if (!beet_bytecode_emit_binary_int(bytecode_function, BEET_BC_OP_MUL_INT,
                                         instr->dst, instr->src_lhs,
                                         instr->src_rhs)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_DIV_INT:
      if (!beet_bytecode_emit_binary_int(bytecode_function, BEET_BC_OP_DIV_INT,
                                         instr->dst, instr->src_lhs,
                                         instr->src_rhs)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_EQ_INT:
      if (!beet_bytecode_emit_binary_int(bytecode_function, BEET_BC_OP_EQ_INT,
                                         instr->dst, instr->src_lhs,
                                         instr->src_rhs)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_NE_INT:
      if (!beet_bytecode_emit_binary_int(bytecode_function, BEET_BC_OP_NE_INT,
                                         instr->dst, instr->src_lhs,
                                         instr->src_rhs)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_LT_INT:
      if (!beet_bytecode_emit_binary_int(bytecode_function, BEET_BC_OP_LT_INT,
                                         instr->dst, instr->src_lhs,
                                         instr->src_rhs)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_LE_INT:
      if (!beet_bytecode_emit_binary_int(bytecode_function, BEET_BC_OP_LE_INT,
                                         instr->dst, instr->src_lhs,
                                         instr->src_rhs)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_GT_INT:
      if (!beet_bytecode_emit_binary_int(bytecode_function, BEET_BC_OP_GT_INT,
                                         instr->dst, instr->src_lhs,
                                         instr->src_rhs)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_GE_INT:
      if (!beet_bytecode_emit_binary_int(bytecode_function, BEET_BC_OP_GE_INT,
                                         instr->dst, instr->src_lhs,
                                         instr->src_rhs)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_LABEL:
      if (!beet_bytecode_emit2(bytecode_function, BEET_BC_OP_LABEL,
                               instr->int_value)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_JUMP:
      if (!beet_bytecode_emit2(bytecode_function, BEET_BC_OP_JUMP,
                               instr->int_value)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_JUMP_IF_FALSE:
      if (!beet_bytecode_emit3(bytecode_function, BEET_BC_OP_JUMP_IF_FALSE,
                               instr->dst, instr->int_value)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_RETURN_LOCAL: {
      int local_index = beet_find_local_index(mir_function, instr->name);
      if (local_index < 0) {
        return 0;
      }

      if (!beet_bytecode_emit2(bytecode_function, BEET_BC_OP_RETURN_LOCAL,
                               local_index)) {
        return 0;
      }
      break;
    }

    case BEET_MIR_OP_RETURN_TEMP:
      if (!beet_bytecode_emit2(bytecode_function, BEET_BC_OP_RETURN_TEMP,
                               instr->dst)) {
        return 0;
      }
      break;

    case BEET_MIR_OP_RETURN_CONST_INT:
      if (!beet_bytecode_emit2(bytecode_function, BEET_BC_OP_RETURN_CONST_INT,
                               instr->int_value)) {
        return 0;
      }
      break;

    default:
      return 0;
    }
  }

  return 1;
}

int beet_codegen_program(const beet_mir_function *mir_functions,
                         size_t function_count,
                         beet_bytecode_program *bytecode_program) {
  size_t i;

  assert(mir_functions != NULL);
  assert(bytecode_program != NULL);

  if (function_count > BEET_BC_MAX_FUNCTIONS) {
    return 0;
  }

  beet_bytecode_program_init(bytecode_program);

  for (i = 0U; i < function_count; ++i) {
    if (!beet_codegen_single_function(&mir_functions[i], mir_functions,
                                      function_count,
                                      &bytecode_program->functions[i])) {
      return 0;
    }
  }

  bytecode_program->function_count = function_count;
  return 1;
}

int beet_codegen_function(const beet_mir_function *mir_function,
                          beet_bytecode_function *bytecode_function) {
  beet_bytecode_program bytecode_program;

  assert(mir_function != NULL);
  assert(bytecode_function != NULL);

  if (!beet_codegen_program(mir_function, 1U, &bytecode_program)) {
    return 0;
  }

  *bytecode_function = bytecode_program.functions[0];
  return 1;
}
