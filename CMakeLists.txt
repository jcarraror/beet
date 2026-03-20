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
)
target_include_directories(beet_compiler_support
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/compiler/stage0-c/include
)
target_link_libraries(beet_compiler_support PUBLIC beet_warnings)

add_executable(beetc0
    compiler/stage0-c/src/main.c
)
target_include_directories(beetc0
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/compiler/stage0-c/include
)
target_link_libraries(beetc0 PRIVATE beet_compiler_support)

add_executable(beet-vm
    runtime/vm/src/main.c
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

add_test(NAME parser_binding COMMAND test_parser_binding)

add_test(NAME smoke_compiler COMMAND test_smoke_compiler)
add_test(NAME smoke_vm COMMAND test_smoke_vm)
add_test(NAME diag_print COMMAND test_diag_print)
add_test(NAME source_file COMMAND test_source_file)
add_test(NAME lexer_tokens COMMAND test_lexer_tokens)