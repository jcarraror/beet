#ifndef BEET_RESOLVE_SCOPE_H
#define BEET_RESOLVE_SCOPE_H

#include <stddef.h>

#define BEET_SCOPE_MAX_SYMBOLS 256
#define BEET_SCOPE_MAX_STACK 32

typedef struct beet_symbol {
    const char *name;
    size_t depth;
    int is_mutable;
} beet_symbol;

typedef struct beet_scope_stack {
    beet_symbol symbols[BEET_SCOPE_MAX_SYMBOLS];
    size_t symbol_count;

    size_t scope_starts[BEET_SCOPE_MAX_STACK];
    size_t scope_depth;
} beet_scope_stack;

void beet_scope_stack_init(beet_scope_stack *stack);

int beet_scope_enter(beet_scope_stack *stack);
int beet_scope_leave(beet_scope_stack *stack);

int beet_scope_bind(
    beet_scope_stack *stack,
    const char *name,
    int is_mutable
);

const beet_symbol *beet_scope_lookup(
    const beet_scope_stack *stack,
    const char *name
);

#endif