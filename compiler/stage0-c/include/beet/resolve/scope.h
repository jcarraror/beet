#ifndef BEET_RESOLVE_SCOPE_H
#define BEET_RESOLVE_SCOPE_H

#include <stddef.h>

#include "beet/parser/ast.h"
#include "beet/semantics/registry.h"

#define BEET_SCOPE_MAX_SYMBOLS 256
#define BEET_SCOPE_MAX_STACK 32

typedef struct beet_symbol {
  const char *name;
  size_t name_len;
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

int beet_scope_bind(beet_scope_stack *stack, const char *name, int is_mutable);
int beet_scope_bind_slice(beet_scope_stack *stack, const char *name,
                          size_t name_len, int is_mutable);

const beet_symbol *beet_scope_lookup(const beet_scope_stack *stack,
                                     const char *name);
const beet_symbol *beet_scope_lookup_slice(const beet_scope_stack *stack,
                                           const char *name, size_t name_len);

int beet_resolve_function_with_decls(beet_ast_function *function_ast,
                                     const beet_ast_function *function_decls,
                                     size_t function_count);
int beet_resolve_function_with_registry(beet_ast_function *function_ast,
                                        const beet_decl_registry *registry);
int beet_resolve_function(beet_ast_function *function_ast);

#endif
