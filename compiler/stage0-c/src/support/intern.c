#include "beet/support/intern.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct beet_intern_entry {
  char *text;
  size_t len;
  struct beet_intern_entry *next;
} beet_intern_entry;

static beet_intern_entry *beet_intern_entries = NULL;
static int beet_intern_cleanup_registered = 0;

static void beet_intern_cleanup(void) {
  beet_intern_entry *entry;

  entry = beet_intern_entries;
  while (entry != NULL) {
    beet_intern_entry *next = entry->next;

    free(entry->text);
    free(entry);
    entry = next;
  }

  beet_intern_entries = NULL;
}

beet_symbol_id beet_intern_slice(const char *text, size_t len) {
  beet_intern_entry *entry;
  char *copy;

  assert(text != NULL);

  for (entry = beet_intern_entries; entry != NULL; entry = entry->next) {
    if (entry->len == len && strncmp(entry->text, text, len) == 0) {
      return entry->text;
    }
  }

  copy = malloc(len + 1U);
  if (copy == NULL) {
    return NULL;
  }

  memcpy(copy, text, len);
  copy[len] = '\0';

  entry = malloc(sizeof(*entry));
  if (entry == NULL) {
    free(copy);
    return NULL;
  }

  if (!beet_intern_cleanup_registered) {
    atexit(beet_intern_cleanup);
    beet_intern_cleanup_registered = 1;
  }

  entry->text = copy;
  entry->len = len;
  entry->next = beet_intern_entries;
  beet_intern_entries = entry;
  return entry->text;
}

int beet_symbol_eq(beet_symbol_id left, beet_symbol_id right) {
  assert(left != NULL);
  assert(right != NULL);

  return left == right;
}

int beet_interned_slice_equals(const char *left, size_t left_len,
                               const char *right, size_t right_len) {
  assert(left != NULL);
  assert(right != NULL);

  if (beet_symbol_eq(left, right)) {
    return left_len == right_len;
  }

  return left_len == right_len && strncmp(left, right, left_len) == 0;
}
