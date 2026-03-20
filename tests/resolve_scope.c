#include <assert.h>
#include <string.h>

#include "beet/resolve/scope.h"

static void test_bind_and_lookup(void) {
  beet_scope_stack stack;
  const beet_symbol *symbol;

  beet_scope_stack_init(&stack);

  assert(beet_scope_bind(&stack, "x", 0));
  assert(beet_scope_bind(&stack, "total", 1));

  symbol = beet_scope_lookup(&stack, "x");
  assert(symbol != NULL);
  assert(strcmp(symbol->name, "x") == 0);
  assert(symbol->depth == 0U);
  assert(symbol->is_mutable == 0);

  symbol = beet_scope_lookup(&stack, "total");
  assert(symbol != NULL);
  assert(strcmp(symbol->name, "total") == 0);
  assert(symbol->depth == 0U);
  assert(symbol->is_mutable == 1);
}

static void test_reject_duplicate_in_same_scope(void) {
  beet_scope_stack stack;

  beet_scope_stack_init(&stack);

  assert(beet_scope_bind(&stack, "x", 0));
  assert(!beet_scope_bind(&stack, "x", 1));
}

static void test_shadow_in_nested_scope(void) {
  beet_scope_stack stack;
  const beet_symbol *symbol;

  beet_scope_stack_init(&stack);

  assert(beet_scope_bind(&stack, "x", 0));
  assert(beet_scope_enter(&stack));
  assert(beet_scope_bind(&stack, "x", 1));

  symbol = beet_scope_lookup(&stack, "x");
  assert(symbol != NULL);
  assert(symbol->depth == 1U);
  assert(symbol->is_mutable == 1);

  assert(beet_scope_leave(&stack));

  symbol = beet_scope_lookup(&stack, "x");
  assert(symbol != NULL);
  assert(symbol->depth == 0U);
  assert(symbol->is_mutable == 0);
}

static void test_lookup_missing(void) {
  beet_scope_stack stack;

  beet_scope_stack_init(&stack);

  assert(beet_scope_lookup(&stack, "missing") == NULL);
}

int main(void) {
  test_bind_and_lookup();
  test_reject_duplicate_in_same_scope();
  test_shadow_in_nested_scope();
  test_lookup_missing();
  return 0;
}