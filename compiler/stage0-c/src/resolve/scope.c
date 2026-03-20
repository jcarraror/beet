#include "beet/resolve/scope.h"

#include <assert.h>
#include <string.h>

void beet_scope_stack_init(beet_scope_stack *stack) {
  assert(stack != NULL);

  stack->symbol_count = 0U;
  stack->scope_depth = 0U;
  stack->scope_starts[0] = 0U;
}

int beet_scope_enter(beet_scope_stack *stack) {
  assert(stack != NULL);

  if (stack->scope_depth + 1U >= BEET_SCOPE_MAX_STACK) {
    return 0;
  }

  stack->scope_depth += 1U;
  stack->scope_starts[stack->scope_depth] = stack->symbol_count;
  return 1;
}

int beet_scope_leave(beet_scope_stack *stack) {
  assert(stack != NULL);

  if (stack->scope_depth == 0U) {
    return 0;
  }

  stack->symbol_count = stack->scope_starts[stack->scope_depth];
  stack->scope_depth -= 1U;
  return 1;
}

int beet_scope_bind(beet_scope_stack *stack, const char *name, int is_mutable) {
  size_t start;
  size_t i;

  assert(stack != NULL);
  assert(name != NULL);

  if (stack->symbol_count >= BEET_SCOPE_MAX_SYMBOLS) {
    return 0;
  }

  start = stack->scope_starts[stack->scope_depth];
  for (i = start; i < stack->symbol_count; ++i) {
    if (strcmp(stack->symbols[i].name, name) == 0) {
      return 0;
    }
  }

  stack->symbols[stack->symbol_count].name = name;
  stack->symbols[stack->symbol_count].depth = stack->scope_depth;
  stack->symbols[stack->symbol_count].is_mutable = is_mutable;
  stack->symbol_count += 1U;
  return 1;
}

const beet_symbol *beet_scope_lookup(const beet_scope_stack *stack,
                                     const char *name) {
  size_t i;

  assert(stack != NULL);
  assert(name != NULL);

  i = stack->symbol_count;
  while (i > 0U) {
    i -= 1U;
    if (strcmp(stack->symbols[i].name, name) == 0) {
      return &stack->symbols[i];
    }
  }

  return NULL;
}