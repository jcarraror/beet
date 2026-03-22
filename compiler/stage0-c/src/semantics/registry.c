#include "beet/semantics/registry.h"

#include <assert.h>

#include "beet/support/intern.h"

void beet_decl_registry_init(beet_decl_registry *registry,
                             const beet_ast_type_decl *type_decls,
                             size_t type_decl_count,
                             const beet_ast_function *function_decls,
                             size_t function_count) {
  assert(registry != NULL);
  assert(type_decls != NULL || type_decl_count == 0U);
  assert(function_decls != NULL || function_count == 0U);

  registry->type_decls = type_decls;
  registry->type_decl_count = type_decl_count;
  registry->function_decls = function_decls;
  registry->function_count = function_count;
}

const beet_ast_type_decl *
beet_decl_registry_find_type(const beet_decl_registry *registry,
                             const char *name, size_t name_len) {
  beet_symbol_id symbol;

  symbol = beet_intern_slice(name, name_len);
  if (symbol == NULL) {
    return NULL;
  }

  return beet_decl_registry_find_type_symbol(registry, symbol);
}

const beet_ast_type_decl *
beet_decl_registry_find_type_symbol(const beet_decl_registry *registry,
                                    beet_symbol_id name) {
  size_t i;

  assert(registry != NULL);
  assert(name != NULL);

  for (i = 0U; i < registry->type_decl_count; ++i) {
    if (beet_symbol_eq(registry->type_decls[i].name, name)) {
      return &registry->type_decls[i];
    }
  }

  return NULL;
}

const beet_ast_function *
beet_decl_registry_find_function(const beet_decl_registry *registry,
                                 const char *name, size_t name_len) {
  beet_symbol_id symbol;

  symbol = beet_intern_slice(name, name_len);
  if (symbol == NULL) {
    return NULL;
  }

  return beet_decl_registry_find_function_symbol(registry, symbol);
}

const beet_ast_function *
beet_decl_registry_find_function_symbol(const beet_decl_registry *registry,
                                        beet_symbol_id name) {
  size_t i;

  assert(registry != NULL);
  assert(name != NULL);

  for (i = 0U; i < registry->function_count; ++i) {
    if (beet_symbol_eq(registry->function_decls[i].name, name)) {
      return &registry->function_decls[i];
    }
  }

  return NULL;
}

int beet_decl_registry_find_function_index(const beet_decl_registry *registry,
                                           const char *name, size_t name_len,
                                           size_t *out_index) {
  beet_symbol_id symbol;

  symbol = beet_intern_slice(name, name_len);
  if (symbol == NULL) {
    return 0;
  }

  return beet_decl_registry_find_function_index_symbol(registry, symbol,
                                                       out_index);
}

int beet_decl_registry_find_function_index_symbol(
    const beet_decl_registry *registry, beet_symbol_id name,
    size_t *out_index) {
  size_t i;

  assert(registry != NULL);
  assert(name != NULL);
  assert(out_index != NULL);

  for (i = 0U; i < registry->function_count; ++i) {
    if (beet_symbol_eq(registry->function_decls[i].name, name)) {
      *out_index = i;
      return 1;
    }
  }

  return 0;
}

const beet_ast_choice_variant *
beet_decl_registry_find_choice_variant(const beet_ast_type_decl *type_decl,
                                       const char *name, size_t name_len) {
  beet_symbol_id symbol;

  symbol = beet_intern_slice(name, name_len);
  if (symbol == NULL) {
    return NULL;
  }

  return beet_decl_registry_find_choice_variant_symbol(type_decl, symbol);
}

const beet_ast_choice_variant *beet_decl_registry_find_choice_variant_symbol(
    const beet_ast_type_decl *type_decl, beet_symbol_id name) {
  size_t i;

  assert(type_decl != NULL);
  assert(name != NULL);

  for (i = 0U; i < type_decl->variant_count; ++i) {
    if (beet_symbol_eq(type_decl->variants[i].name, name)) {
      return &type_decl->variants[i];
    }
  }

  return NULL;
}

int beet_decl_registry_find_choice_variant_index(
    const beet_ast_type_decl *type_decl, const char *name, size_t name_len,
    size_t *out_index) {
  beet_symbol_id symbol;

  symbol = beet_intern_slice(name, name_len);
  if (symbol == NULL) {
    return 0;
  }

  return beet_decl_registry_find_choice_variant_index_symbol(type_decl, symbol,
                                                             out_index);
}

int beet_decl_registry_find_choice_variant_index_symbol(
    const beet_ast_type_decl *type_decl, beet_symbol_id name,
    size_t *out_index) {
  size_t i;

  assert(type_decl != NULL);
  assert(name != NULL);
  assert(out_index != NULL);

  for (i = 0U; i < type_decl->variant_count; ++i) {
    if (beet_symbol_eq(type_decl->variants[i].name, name)) {
      *out_index = i;
      return 1;
    }
  }

  return 0;
}
