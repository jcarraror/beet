#ifndef BEET_CODEGEN_CODEGEN_H
#define BEET_CODEGEN_CODEGEN_H

#include "beet/mir/mir.h"
#include "beet/vm/bytecode.h"

int beet_codegen_function(const beet_mir_function *mir_function,
                          beet_bytecode_function *bytecode_function);

#endif