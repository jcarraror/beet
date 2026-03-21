#include "beet/semantics/registry.h"

#include <assert.h>
#include <string.h>

static int beet_name_equals_slice(const char *left, size_t left_len,
                                  const char *right, size_t right_len) {
  return left_len == right_len && strncmp(left, right, left_len) == 0;
}

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
  size_t i;

  assert(registry != NULL);
  assert(name != NULL);

  for (i = 0U; i < registry->type_decl_count; ++i) {
    if (beet_name_equals_slice(registry->type_decls[i].name,
                               registry->type_decls[i].name_len, name,
                               name_len)) {
      return &registry->type_decls[i];
    }
  }

  return NULL;
}

const beet_ast_function *
beet_decl_registry_find_function(const beet_decl_registry *registry,
                                 const char *name, size_t name_len) {
  size_t i;

  assert(registry != NULL);
  assert(name != NULL);

  for (i = 0U; i < registry->function_count; ++i) {
    if (beet_name_equals_slice(registry->function_decls[i].name,
                               registry->function_decls[i].name_len, name,
                               name_len)) {
      return &registry->function_decls[i];
    }
  }

  return NULL;
}

int beet_decl_registry_find_function_index(const beet_decl_registry *registry,
                                           const char *name, size_t name_len,
                                           size_t *out_index) {
  size_t i;

  assert(registry != NULL);
  assert(name != NULL);
  assert(out_index != NULL);

  for (i = 0U; i < registry->function_count; ++i) {
    if (beet_name_equals_slice(registry->function_decls[i].name,
                               registry->function_decls[i].name_len, name,
                               name_len)) {
      *out_index = i;
      return 1;
    }
  }

  return 0;
}

const beet_ast_choice_variant *
beet_decl_registry_find_choice_variant(const beet_ast_type_decl *type_decl,
                                       const char *name, size_t name_len) {
  size_t i;

  assert(type_decl != NULL);
  assert(name != NULL);

  for (i = 0U; i < type_decl->variant_count; ++i) {
    if (beet_name_equals_slice(type_decl->variants[i].name,
                               type_decl->variants[i].name_len, name,
                               name_len)) {
      return &type_decl->variants[i];
    }
  }

  return NULL;
}

int beet_decl_registry_find_choice_variant_index(
    const beet_ast_type_decl *type_decl, const char *name, size_t name_len,
    size_t *out_index) {
  size_t i;

  assert(type_decl != NULL);
  assert(name != NULL);
  assert(out_index != NULL);

  for (i = 0U; i < type_decl->variant_count; ++i) {
    if (beet_name_equals_slice(type_decl->variants[i].name,
                               type_decl->variants[i].name_len, name,
                               name_len)) {
      *out_index = i;
      return 1;
    }
  }

  return 0;
}
