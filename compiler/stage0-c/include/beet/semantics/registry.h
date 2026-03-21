#ifndef BEET_SEMANTICS_REGISTRY_H
#define BEET_SEMANTICS_REGISTRY_H

#include <stddef.h>

#include "beet/parser/ast.h"

typedef struct beet_decl_registry {
  const beet_ast_type_decl *type_decls;
  size_t type_decl_count;
  const beet_ast_function *function_decls;
  size_t function_count;
} beet_decl_registry;

void beet_decl_registry_init(beet_decl_registry *registry,
                             const beet_ast_type_decl *type_decls,
                             size_t type_decl_count,
                             const beet_ast_function *function_decls,
                             size_t function_count);

const beet_ast_type_decl *
beet_decl_registry_find_type(const beet_decl_registry *registry,
                             const char *name, size_t name_len);

const beet_ast_function *
beet_decl_registry_find_function(const beet_decl_registry *registry,
                                 const char *name, size_t name_len);

int beet_decl_registry_find_function_index(const beet_decl_registry *registry,
                                           const char *name, size_t name_len,
                                           size_t *out_index);

const beet_ast_choice_variant *
beet_decl_registry_find_choice_variant(const beet_ast_type_decl *type_decl,
                                       const char *name, size_t name_len);

int beet_decl_registry_find_choice_variant_index(
    const beet_ast_type_decl *type_decl, const char *name, size_t name_len,
    size_t *out_index);

#endif
