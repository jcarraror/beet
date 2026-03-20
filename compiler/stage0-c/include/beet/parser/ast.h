#ifndef BEET_PARSER_AST_H
#define BEET_PARSER_AST_H

#include <stddef.h>

#define BEET_AST_MAX_PARAMS 16
#define BEET_AST_MAX_FIELDS 32

typedef struct beet_ast_binding {
    const char *name;

    int is_mutable;
    int has_type;

    const char *type_name; /* optional */

    const char *value_text;
    size_t value_len;
} beet_ast_binding;

typedef struct beet_ast_param {
    const char *name;
    const char *type_name;
} beet_ast_param;

typedef struct beet_ast_function {
    const char *name;
    const char *return_type_name;

    beet_ast_param params[BEET_AST_MAX_PARAMS];
    size_t param_count;
} beet_ast_function;

typedef struct beet_ast_field {
    const char *name;
    const char *type_name;
} beet_ast_field;

typedef struct beet_ast_type_decl {
    const char *name;
    int is_structure;

    beet_ast_field fields[BEET_AST_MAX_FIELDS];
    size_t field_count;
} beet_ast_type_decl;

#endif