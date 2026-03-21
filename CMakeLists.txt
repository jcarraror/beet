cmake_minimum_required(VERSION 3.20)

project(beet C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)
enable_testing()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(warnings)

add_library(beet_warnings INTERFACE)
target_compile_options(beet_warnings INTERFACE ${BEET_WARNING_FLAGS})

add_library(beet_compiler_support
    compiler/stage0-c/src/support/source.c
    compiler/stage0-c/src/diag/diag.c
    compiler/stage0-c/src/lexer/lexer.c
    compiler/stage0-c/src/parser/parser.c
    compiler/stage0-c/src/resolve/scope.c
    compiler/stage0-c/src/types/types.c
    compiler/stage0-c/src/types/check.c
    compiler/stage0-c/src/mir/mir.c
    compiler/stage0-c/src/codegen/codegen.c
    runtime/vm/src/bytecode.c
    runtime/vm/src/interpreter.c
)
target_include_directories(beet_compiler_support
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/compiler/stage0-c/include
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime/vm/include
)
target_link_libraries(beet_compiler_support PUBLIC beet_warnings)

add_executable(beetc0
    compiler/stage0-c/src/main.c
)
target_include_directories(beetc0
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/compiler/stage0-c/include
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime/vm/include
)
target_link_libraries(beetc0 PRIVATE beet_compiler_support)

add_executable(beet-vm
    runtime/vm/src/main.c
    runtime/vm/src/bytecode.c
    runtime/vm/src/interpreter.c
)
target_include_directories(beet-vm
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime/vm/include
)
target_link_libraries(beet-vm PRIVATE beet_warnings)

add_executable(test_smoke_compiler
    tests/smoke_compiler.c
)
target_link_libraries(test_smoke_compiler PRIVATE beet_warnings)

add_executable(test_smoke_vm
    tests/smoke_vm.c
)
target_link_libraries(test_smoke_vm PRIVATE beet_warnings)

add_executable(test_diag_print
    tests/diag_print.c
)
target_link_libraries(test_diag_print PRIVATE beet_compiler_support)

add_executable(test_source_file
    tests/source_file.c
)
target_link_libraries(test_source_file PRIVATE beet_compiler_support)

add_executable(test_lexer_tokens
    tests/lexer_tokens.c
)
target_link_libraries(test_lexer_tokens PRIVATE beet_compiler_support)

add_executable(test_parser_binding
    tests/parser_binding.c
)
target_link_libraries(test_parser_binding PRIVATE beet_compiler_support)

add_executable(test_parser_function
    tests/parser_function.c
)
target_link_libraries(test_parser_function PRIVATE beet_compiler_support)

add_executable(test_parser_type_decl
    tests/parser_type_decl.c
)
target_link_libraries(test_parser_type_decl PRIVATE beet_compiler_support)

add_executable(test_resolve_scope
    tests/resolve_scope.c
)
target_link_libraries(test_resolve_scope PRIVATE beet_compiler_support)

add_executable(test_types_check
    tests/types_check.c
)
target_link_libraries(test_types_check PRIVATE beet_compiler_support)

add_executable(test_mir_lowering
    tests/mir_lowering.c
)
target_link_libraries(test_mir_lowering PRIVATE beet_compiler_support)

add_executable(test_codegen_bytecode
    tests/codegen_bytecode.c
)
target_link_libraries(test_codegen_bytecode PRIVATE beet_compiler_support)

add_executable(test_vm_execution
    tests/vm_execution.c
)
target_link_libraries(test_vm_execution PRIVATE beet_compiler_support)

add_test(NAME smoke_compiler COMMAND test_smoke_compiler)
add_test(NAME smoke_vm COMMAND test_smoke_vm)
add_test(NAME diag_print COMMAND test_diag_print)
add_test(NAME source_file COMMAND test_source_file)
add_test(NAME lexer_tokens COMMAND test_lexer_tokens)
add_test(NAME parser_binding COMMAND test_parser_binding)
add_test(NAME parser_function COMMAND test_parser_function)
add_test(NAME parser_type_decl COMMAND test_parser_type_decl)
add_test(NAME resolve_scope COMMAND test_resolve_scope)
add_test(NAME types_check COMMAND test_types_check)
add_test(NAME mir_lowering COMMAND test_mir_lowering)
add_test(NAME codegen_bytecode COMMAND test_codegen_bytecode)
add_test(NAME vm_execution COMMAND test_vm_execution)

add_test(
    NAME driver_return_zero
    COMMAND beetc0 ${CMAKE_CURRENT_SOURCE_DIR}/examples/return_zero.beet
)
set_tests_properties(
    driver_return_zero
    PROPERTIES
        PASS_REGULAR_EXPRESSION "0"
)

add_test(
    NAME driver_return_seven
    COMMAND beetc0 ${CMAKE_CURRENT_SOURCE_DIR}/examples/return_seven.beet
)
set_tests_properties(
    driver_return_seven
    PROPERTIES
        PASS_REGULAR_EXPRESSION "7"
)

add_test(
    NAME driver_return_bound_local
    COMMAND beetc0 ${CMAKE_CURRENT_SOURCE_DIR}/examples/return_bound_local.beet
)
set_tests_properties(
    driver_return_bound_local
    PROPERTIES
        PASS_REGULAR_EXPRESSION "10"
)

add_test(
    NAME driver_return_arithmetic
    COMMAND beetc0 ${CMAKE_CURRENT_SOURCE_DIR}/examples/return_arithmetic.beet
)
set_tests_properties(
    driver_return_arithmetic
    PROPERTIES
        PASS_REGULAR_EXPRESSION "-3"
)


add_test(
    NAME driver_return_if_true
    COMMAND beetc0 ${CMAKE_CURRENT_SOURCE_DIR}/examples/return_if_true.beet
)
set_tests_properties(
    driver_return_if_true
    PROPERTIES
        PASS_REGULAR_EXPRESSION "7"
)
