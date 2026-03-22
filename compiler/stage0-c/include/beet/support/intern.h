#ifndef BEET_SUPPORT_INTERN_H
#define BEET_SUPPORT_INTERN_H

#include <stddef.h>

typedef const char *beet_symbol_id;

beet_symbol_id beet_intern_slice(const char *text, size_t len);
int beet_symbol_eq(beet_symbol_id left, beet_symbol_id right);
int beet_interned_slice_equals(const char *left, size_t left_len,
                               const char *right, size_t right_len);

#endif
