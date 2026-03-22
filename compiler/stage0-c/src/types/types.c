#include "beet/types/types.h"

#include <assert.h>

#include "beet/support/intern.h"

static beet_symbol_id beet_builtin_symbol(const char *name, size_t len) {
  beet_symbol_id symbol;

  symbol = beet_intern_slice(name, len);
  assert(symbol != NULL);
  return symbol;
}

beet_type beet_type_from_name_slice(const char *name, size_t len) {
  beet_type type;
  beet_symbol_id symbol;
  beet_symbol_id int_symbol;
  beet_symbol_id float_symbol;
  beet_symbol_id bool_symbol;
  beet_symbol_id unit_symbol;

  assert(name != NULL);

  symbol = beet_intern_slice(name, len);
  assert(symbol != NULL);

  type.kind = BEET_TYPE_NAMED;
  type.name = symbol;
  type.name_len = len;

  int_symbol = beet_builtin_symbol("Int", 3U);
  float_symbol = beet_builtin_symbol("Float", 5U);
  bool_symbol = beet_builtin_symbol("Bool", 4U);
  unit_symbol = beet_builtin_symbol("Unit", 4U);

  if (beet_symbol_eq(symbol, int_symbol)) {
    type.kind = BEET_TYPE_INT;
    type.name = int_symbol;
    type.name_len = 3U;
    return type;
  }

  if (beet_symbol_eq(symbol, float_symbol)) {
    type.kind = BEET_TYPE_FLOAT;
    type.name = float_symbol;
    type.name_len = 5U;
    return type;
  }

  if (beet_symbol_eq(symbol, bool_symbol)) {
    type.kind = BEET_TYPE_BOOL;
    type.name = bool_symbol;
    type.name_len = 4U;
    return type;
  }

  if (beet_symbol_eq(symbol, unit_symbol)) {
    type.kind = BEET_TYPE_UNIT;
    type.name = unit_symbol;
    type.name_len = 4U;
    return type;
  }

  return type;
}

int beet_type_is_known(const beet_type *type) {
  assert(type != NULL);

  return type->kind != BEET_TYPE_INVALID && type->kind != BEET_TYPE_NAMED;
}
