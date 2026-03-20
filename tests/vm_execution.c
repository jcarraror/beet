#include <assert.h>

#include "beet/vm/interpreter.h"
#include "beet/vm/bytecode.h"

static void test_return_const(void) {
    beet_bytecode_function fn;
    beet_vm vm;
    int result;

    beet_bytecode_function_init(&fn);
    assert(beet_bytecode_emit_return_const_int(&fn, 42));

    assert(beet_vm_execute(&vm, &fn, &result));
    assert(result == 42);
}

static void test_store_and_return_local(void) {
    beet_bytecode_function fn;
    beet_vm vm;
    int result;

    beet_bytecode_function_init(&fn);

    assert(beet_bytecode_emit_const_int(&fn, 0, 10));
    assert(beet_bytecode_emit_store_local(&fn, 0, 0));
    assert(beet_bytecode_emit_return_local(&fn, 0));

    assert(beet_vm_execute(&vm, &fn, &result));
    assert(result == 10);
}

static void test_multiple_values(void) {
    beet_bytecode_function fn;
    beet_vm vm;
    int result;

    beet_bytecode_function_init(&fn);

    assert(beet_bytecode_emit_const_int(&fn, 0, 5));
    assert(beet_bytecode_emit_const_int(&fn, 1, 7));
    assert(beet_bytecode_emit_store_local(&fn, 0, 1));
    assert(beet_bytecode_emit_return_local(&fn, 0));

    assert(beet_vm_execute(&vm, &fn, &result));
    assert(result == 7);
}

int main(void) {
    test_return_const();
    test_store_and_return_local();
    test_multiple_values();
    return 0;
}