#include "beet/types/check.h"

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>

static beet_type beet_type_from_value_text(const char *text, size_t len) {
    beet_type type;
    size_t i;
    int saw_dot = 0;

    assert(text != NULL);

    type.kind = BEET_TYPE_INVALID;
    type.name = NULL;

    if (len == 4U && strncmp(text, "true", 4U) == 0) {
        type.kind = BEET_TYPE_BOOL;
        type.name = "Bool";
        return type;
    }

    if (len == 5U && strncmp(text, "false", 5U) == 0) {
        type.kind = BEET_TYPE_BOOL;
        type.name = "Bool";
        return type;
    }

    if (len == 2U && strncmp(text, "()", 2U) == 0) {
        type.kind = BEET_TYPE_UNIT;
        type.name = "Unit";
        return type;
    }

    if (len == 0U) {
        return type;
    }

    for (i = 0U; i < len; ++i) {
        if (text[i] == '.') {
            if (saw_dot) {
                return type;
            }
            saw_dot = 1;
            continue;
        }

        if (!isdigit((unsigned char)text[i])) {
            return type;
        }
    }

    if (saw_dot) {
        type.kind = BEET_TYPE_FLOAT;
        type.name = "Float";
    } else {
        type.kind = BEET_TYPE_INT;
        type.name = "Int";
    }

    return type;
}

beet_type beet_type_check_binding(const beet_ast_binding *binding) {
    assert(binding != NULL);

    return beet_type_from_value_text(binding->value_text, binding->value_len);
}

beet_type_check_result beet_type_check_binding_annotation(
    const beet_ast_binding *binding
) {
    beet_type_check_result result;

    assert(binding != NULL);

    result.ok = 1;
    result.declared_type.kind = BEET_TYPE_INVALID;
    result.declared_type.name = NULL;
    result.value_type = beet_type_check_binding(binding);

    if (!binding->has_type) {
        return result;
    }

    result.declared_type = beet_type_from_name_slice(
        binding->type_name,
        binding->type_name_len
    );

    if (!beet_type_is_known(&result.value_type)) {
        result.ok = 0;
        return result;
    }

    if (result.declared_type.kind != result.value_type.kind) {
        result.ok = 0;
    }

    return result;
}

int beet_type_check_function_signature(const beet_ast_function *function_ast) {
    size_t i;
    beet_type return_type;

    assert(function_ast != NULL);

    return_type = beet_type_from_name_slice(
        function_ast->return_type_name,
        function_ast->return_type_name_len
    );
    if (!beet_type_is_known(&return_type)) {
        return 0;
    }

    for (i = 0U; i < function_ast->param_count; ++i) {
        beet_type param_type = beet_type_from_name_slice(
            function_ast->params[i].type_name,
            function_ast->params[i].type_name_len
        );
        if (!beet_type_is_known(&param_type)) {
            return 0;
        }
    }

    return 1;
}

int beet_type_check_type_decl(const beet_ast_type_decl *type_decl) {
    size_t i;

    assert(type_decl != NULL);

    for (i = 0U; i < type_decl->field_count; ++i) {
        beet_type field_type = beet_type_from_name_slice(
            type_decl->fields[i].type_name,
            type_decl->fields[i].type_name_len
        );
        if (!beet_type_is_known(&field_type)) {
            return 0;
        }
    }

    return 1;
}