#ifndef BEET_TYPES_TYPES_H
#define BEET_TYPES_TYPES_H

#include <stddef.h>

typedef enum beet_type_kind {
    BEET_TYPE_INVALID = 0,
    BEET_TYPE_INT,
    BEET_TYPE_FLOAT,
    BEET_TYPE_BOOL,
    BEET_TYPE_UNIT,
    BEET_TYPE_NAMED
} beet_type_kind;

typedef struct beet_type {
    beet_type_kind kind;
    const char *name;
} beet_type;

beet_type beet_type_from_name_slice(const char *name, size_t len);
int beet_type_is_known(const beet_type *type);

#endif