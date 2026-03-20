#include "beet/vm/bytecode.h"

#include <assert.h>

static int beet_bytecode_emit3(beet_bytecode_function *function, int a, int b,
                               int c) {
  assert(function != NULL);

  if (function->code_count + 3U > BEET_BC_MAX_CODE) {
    return 0;
  }

  function->code[function->code_count++] = a;
  function->code[function->code_count++] = b;
  function->code[function->code_count++] = c;
  return 1;
}

static int beet_bytecode_emit2(beet_bytecode_function *function, int a, int b) {
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

  function->code_count = 0U;
  function->local_count = 0U;
}

int beet_bytecode_emit_const_int(beet_bytecode_function *function, int dst,
                                 int value) {
  return beet_bytecode_emit3(function, BEET_BC_OP_CONST_INT, dst, value);
}

int beet_bytecode_emit_store_local(beet_bytecode_function *function,
                                   int local_index, int src_temp) {
  if ((size_t)(local_index + 1) > function->local_count) {
    function->local_count = (size_t)(local_index + 1);
  }

  return beet_bytecode_emit3(function, BEET_BC_OP_STORE_LOCAL, local_index,
                             src_temp);
}

int beet_bytecode_emit_return_local(beet_bytecode_function *function,
                                    int local_index) {
  return beet_bytecode_emit2(function, BEET_BC_OP_RETURN_LOCAL, local_index);
}

int beet_bytecode_emit_return_const_int(beet_bytecode_function *function,
                                        int value) {
  return beet_bytecode_emit2(function, BEET_BC_OP_RETURN_CONST_INT, value);
}