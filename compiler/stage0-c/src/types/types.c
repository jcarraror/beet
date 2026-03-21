#include "beet/types/types.h"

#include <assert.h>
#include <string.h>

static int beet_name_equals(const char *name, size_t len,
                            const char *expected) {
  size_t expected_len;

  assert(name != NULL);
  assert(expected != NULL);

  expected_len = strlen(expected);
  return expected_len == len && strncmp(name, expected, len) == 0;
}

beet_type beet_type_from_name_slice(const char *name, size_t len) {
  beet_type type;

  assert(name != NULL);

  type.kind = BEET_TYPE_NAMED;
  type.name = name;

  if (beet_name_equals(name, len, "Int")) {
    type.kind = BEET_TYPE_INT;
    return type;
  }

  if (beet_name_equals(name, len, "Float")) {
    type.kind = BEET_TYPE_FLOAT;
    return type;
  }

  if (beet_name_equals(name, len, "Bool")) {
    type.kind = BEET_TYPE_BOOL;
    return type;
  }

  if (beet_name_equals(name, len, "Unit")) {
    type.kind = BEET_TYPE_UNIT;
    return type;
  }

  return type;
}

int beet_type_is_known(const beet_type *type) {
  assert(type != NULL);

  return type->kind != BEET_TYPE_INVALID && type->kind != BEET_TYPE_NAMED;
}