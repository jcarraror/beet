#include "beet/types/types.h"

#include <assert.h>
#include <string.h>

#include "beet/support/intern.h"

static int beet_name_equals(const char *name, size_t len,
                            const char *expected) {
  return beet_interned_slice_equals(name, len, expected, strlen(expected));
}

static beet_symbol_id beet_builtin_symbol(const char *name, size_t len) {
  beet_symbol_id symbol;

  symbol = beet_intern_slice(name, len);
  assert(symbol != NULL);
  return symbol;
}

beet_type beet_type_from_name_slice(const char *name, size_t len) {
  beet_type type;

  assert(name != NULL);

  type.kind = BEET_TYPE_NAMED;
  type.name = beet_intern_slice(name, len);
  assert(type.name != NULL);
  type.name_len = len;

  if (beet_name_equals(name, len, "Int")) {
    type.kind = BEET_TYPE_INT;
    type.name = beet_builtin_symbol("Int", 3U);
    type.name_len = 3U;
    return type;
  }

  if (beet_name_equals(name, len, "Float")) {
    type.kind = BEET_TYPE_FLOAT;
    type.name = beet_builtin_symbol("Float", 5U);
    type.name_len = 5U;
    return type;
  }

  if (beet_name_equals(name, len, "Bool")) {
    type.kind = BEET_TYPE_BOOL;
    type.name = beet_builtin_symbol("Bool", 4U);
    type.name_len = 4U;
    return type;
  }

  if (beet_name_equals(name, len, "Unit")) {
    type.kind = BEET_TYPE_UNIT;
    type.name = beet_builtin_symbol("Unit", 4U);
    type.name_len = 4U;
    return type;
  }

  return type;
}

int beet_type_is_known(const beet_type *type) {
  assert(type != NULL);

  return type->kind != BEET_TYPE_INVALID && type->kind != BEET_TYPE_NAMED;
}
