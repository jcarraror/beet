#ifndef BEET_PARSER_AST_H
#define BEET_PARSER_AST_H

#include <stddef.h>

typedef struct beet_ast_binding {
    const char *name;

    int is_mutable;
    int has_type;

    const char *type_name; /* optional */

    const char *value_text;
    size_t value_len;
} beet_ast_binding;

#endif