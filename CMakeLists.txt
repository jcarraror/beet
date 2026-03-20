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

add_executable(beetc0
    compiler/stage0-c/src/main.c
)
target_include_directories(beetc0
    PRIVATE
        compiler/stage0-c/include
)
target_link_libraries(beetc0 PRIVATE beet_warnings)

add_executable(beet-vm
    runtime/vm/src/main.c
)
target_include_directories(beet-vm
    PRIVATE
        runtime/vm/include
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

add_test(NAME smoke_compiler COMMAND test_smoke_compiler)
add_test(NAME smoke_vm COMMAND test_smoke_vm)