#include <assert.h>

#include "beet/vm/bytecode.h"
#include "beet/vm/interpreter.h"

static void test_return_const(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 42));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 42);
}

static void test_store_and_return_local(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 10));
  assert(beet_bytecode_emit_store_local(&fn, 0, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_LOCAL, 0));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 10);
}

static void test_multiple_values(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 5));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 1, 7));
  assert(beet_bytecode_emit_store_local(&fn, 0, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_LOCAL, 0));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 7);
}

static void test_arithmetic_return_temp(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 1));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 1, 2));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 2, 3));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_MUL_INT, 3, 1, 2));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_ADD_INT, 4, 0, 3));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_TEMP, 4));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 7);
}

static void test_load_local_and_return_temp(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 10));
  assert(beet_bytecode_emit_store_local(&fn, 0, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_LOAD_LOCAL, 1, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 2, 2));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_ADD_INT, 3, 1, 2));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_TEMP, 3));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 12);
}

static void test_divide_by_zero_fails(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 8));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 1, 0));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_DIV_INT, 2, 0, 1));
  assert(!beet_vm_execute(&vm, &fn, &result));
}

static void test_jump_skips_instructions(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_JUMP, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 99));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 7));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 7);
}

static void test_jump_if_false_takes_branch(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_JUMP_IF_FALSE, 0, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 2));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 2);
}

static void test_jump_if_false_falls_through_on_true(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 1));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_JUMP_IF_FALSE, 0, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 2));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 1);
}

static void test_jump_to_missing_label_fails(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_JUMP, 9));
  assert(!beet_vm_execute(&vm, &fn, &result));
}

int main(void) {
  test_return_const();
  test_store_and_return_local();
  test_multiple_values();
  test_arithmetic_return_temp();
  test_load_local_and_return_temp();
  test_divide_by_zero_fails();
  test_jump_skips_instructions();
  test_jump_if_false_takes_branch();
  test_jump_if_false_falls_through_on_true();
  test_jump_to_missing_label_fails();
  return 0;
}
