#ifndef BEET_SUPPORT_INTERN_H
#define BEET_SUPPORT_INTERN_H

#include <stddef.h>

const char *beet_intern_slice(const char *text, size_t len);
int beet_interned_slice_equals(const char *left, size_t left_len,
                               const char *right, size_t right_len);

#endif
