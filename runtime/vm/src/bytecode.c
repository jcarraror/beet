#include "beet/vm/bytecode.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct beet_bytecode_allocation {
  void *ptr;
  struct beet_bytecode_allocation *next;
} beet_bytecode_allocation;

static beet_bytecode_allocation *beet_bytecode_allocations = NULL;
static int beet_bytecode_cleanup_registered = 0;

static void beet_bytecode_cleanup_allocations(void) {
  beet_bytecode_allocation *allocation;

  allocation = beet_bytecode_allocations;
  while (allocation != NULL) {
    beet_bytecode_allocation *next = allocation->next;

    free(allocation->ptr);
    free(allocation);
    allocation = next;
  }

  beet_bytecode_allocations = NULL;
}

static void *beet_bytecode_alloc_zeroed(size_t size) {
  beet_bytecode_allocation *allocation;
  void *ptr;

  if (size == 0U) {
    return NULL;
  }

  ptr = calloc(1U, size);
  if (ptr == NULL) {
    return NULL;
  }

  allocation = malloc(sizeof(*allocation));
  if (allocation == NULL) {
    free(ptr);
    return NULL;
  }

  if (!beet_bytecode_cleanup_registered) {
    atexit(beet_bytecode_cleanup_allocations);
    beet_bytecode_cleanup_registered = 1;
  }

  allocation->ptr = ptr;
  allocation->next = beet_bytecode_allocations;
  beet_bytecode_allocations = allocation;
  return ptr;
}

int beet_bytecode_emit4(beet_bytecode_function *function, int a, int b, int c,
                        int d) {
  assert(function != NULL);

  if (function->code_count + 4U > BEET_BC_MAX_CODE) {
    return 0;
  }

  function->code[function->code_count++] = a;
  function->code[function->code_count++] = b;
  function->code[function->code_count++] = c;
  function->code[function->code_count++] = d;
  return 1;
}

int beet_bytecode_emit3(beet_bytecode_function *function, int a, int b, int c) {
  assert(function != NULL);

  if (function->code_count + 3U > BEET_BC_MAX_CODE) {
    return 0;
  }

  function->code[function->code_count++] = a;
  function->code[function->code_count++] = b;
  function->code[function->code_count++] = c;
  return 1;
}

int beet_bytecode_emit2(beet_bytecode_function *function, int a, int b) {
  assert(function != NULL);

  if (function->code_count + 2U > BEET_BC_MAX_CODE) {
    return 0;
  }

  function->code[function->code_count++] = a;
  function->code[function->code_count++] = b;
  return 1;
}

void beet_bytecode_function_init(beet_bytecode_function *function) {
  assert(function != NULL);

  memset(function, 0, sizeof(*function));
  function->code = beet_bytecode_alloc_zeroed(sizeof(int) * BEET_BC_MAX_CODE);
  assert(function->code != NULL);
  function->code_count = 0U;
  function->local_count = 0U;
  function->param_count = 0U;
}

void beet_bytecode_program_init(beet_bytecode_program *program) {
  assert(program != NULL);

  memset(program, 0, sizeof(*program));
  program->functions = beet_bytecode_alloc_zeroed(
      sizeof(beet_bytecode_function) * BEET_BC_MAX_FUNCTIONS);
  assert(program->functions != NULL);
  program->function_count = 0U;
}

int beet_bytecode_emit_store_local(beet_bytecode_function *function,
                                   int local_index, int src_temp) {
  if ((size_t)(local_index + 1) > function->local_count) {
    function->local_count = (size_t)(local_index + 1);
  }

  return beet_bytecode_emit3(function, BEET_BC_OP_STORE_LOCAL, local_index,
                             src_temp);
}

int beet_bytecode_emit_call(beet_bytecode_function *function, int dst_temp,
                            int function_index, const int *args,
                            size_t arg_count) {
  size_t i;

  assert(function != NULL);
  assert(args != NULL || arg_count == 0U);

  if (function->code_count + 4U + arg_count > BEET_BC_MAX_CODE) {
    return 0;
  }

  function->code[function->code_count++] = BEET_BC_OP_CALL;
  function->code[function->code_count++] = dst_temp;
  function->code[function->code_count++] = function_index;
  function->code[function->code_count++] = (int)arg_count;

  for (i = 0U; i < arg_count; ++i) {
    function->code[function->code_count++] = args[i];
  }

  return 1;
}

int beet_bytecode_emit_binary_int(beet_bytecode_function *function,
                                  beet_bc_opcode opcode, int dst_temp,
                                  int lhs_temp, int rhs_temp) {
  if (opcode != BEET_BC_OP_ADD_INT && opcode != BEET_BC_OP_SUB_INT &&
      opcode != BEET_BC_OP_MUL_INT && opcode != BEET_BC_OP_DIV_INT &&
      opcode != BEET_BC_OP_EQ_INT && opcode != BEET_BC_OP_NE_INT &&
      opcode != BEET_BC_OP_LT_INT && opcode != BEET_BC_OP_LE_INT &&
      opcode != BEET_BC_OP_GT_INT && opcode != BEET_BC_OP_GE_INT) {
    return 0;
  }

  return beet_bytecode_emit4(function, opcode, dst_temp, lhs_temp, rhs_temp);
}
