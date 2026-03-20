#include "beet/codegen/codegen.h"

#include <assert.h>
#include <string.h>

static int beet_find_local_index(
    const beet_mir_function *function,
    const char *name
) {
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

int beet_codegen_function(
    const beet_mir_function *mir_function,
    beet_bytecode_function *bytecode_function
) {
    size_t i;

    assert(mir_function != NULL);
    assert(bytecode_function != NULL);

    beet_bytecode_function_init(bytecode_function);

    for (i = 0U; i < mir_function->instr_count; ++i) {
        const beet_mir_instr *instr = &mir_function->instrs[i];

        switch (instr->op) {
            case BEET_MIR_OP_CONST_INT:
                if (!beet_bytecode_emit_const_int(
                        bytecode_function,
                        instr->dst,
                        instr->int_value)) {
                    return 0;
                }
                break;

            case BEET_MIR_OP_BIND_LOCAL: {
                int local_index = beet_find_local_index(mir_function, instr->name);
                if (local_index < 0) {
                    return 0;
                }

                if (!beet_bytecode_emit_store_local(
                        bytecode_function,
                        local_index,
                        instr->dst)) {
                    return 0;
                }
                break;
            }

            case BEET_MIR_OP_RETURN_LOCAL: {
                int local_index = beet_find_local_index(mir_function, instr->name);
                if (local_index < 0) {
                    return 0;
                }

                if (!beet_bytecode_emit_return_local(bytecode_function, local_index)) {
                    return 0;
                }
                break;
            }

            case BEET_MIR_OP_RETURN_CONST_INT:
                if (!beet_bytecode_emit_return_const_int(
                        bytecode_function,
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