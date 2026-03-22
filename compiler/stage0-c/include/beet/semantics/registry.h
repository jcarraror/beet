#ifndef BEET_SEMANTICS_REGISTRY_H
#define BEET_SEMANTICS_REGISTRY_H

#include <stddef.h>

#include "beet/parser/ast.h"
#include "beet/support/intern.h"

typedef struct beet_decl_registry {
  const beet_ast_type_decl *type_decls;
  size_t type_decl_count;
  const beet_ast_function *function_decls;
  size_t function_count;
} beet_decl_registry;

typedef struct beet_module_symbols {
  const char *name;
  size_t name_len;
  beet_source_span span;
  beet_decl_registry decls;
} beet_module_symbols;

void beet_decl_registry_init(beet_decl_registry *registry,
                             const beet_ast_type_decl *type_decls,
                             size_t type_decl_count,
                             const beet_ast_function *function_decls,
                             size_t function_count);

void beet_module_symbols_init(beet_module_symbols *symbols,
                              const beet_ast_module *module);
int beet_module_symbols_validate(const beet_module_symbols *symbols);

const beet_ast_type_decl *
beet_decl_registry_find_type(const beet_decl_registry *registry,
                             const char *name, size_t name_len);
const beet_ast_type_decl *
beet_decl_registry_find_type_symbol(const beet_decl_registry *registry,
                                    beet_symbol_id name);

const beet_ast_function *
beet_decl_registry_find_function(const beet_decl_registry *registry,
                                 const char *name, size_t name_len);
const beet_ast_function *
beet_decl_registry_find_function_symbol(const beet_decl_registry *registry,
                                        beet_symbol_id name);

int beet_decl_registry_find_function_index(const beet_decl_registry *registry,
                                           const char *name, size_t name_len,
                                           size_t *out_index);
int beet_decl_registry_find_function_index_symbol(
    const beet_decl_registry *registry, beet_symbol_id name, size_t *out_index);

const beet_ast_choice_variant *
beet_decl_registry_find_choice_variant(const beet_ast_type_decl *type_decl,
                                       const char *name, size_t name_len);
const beet_ast_choice_variant *beet_decl_registry_find_choice_variant_symbol(
    const beet_ast_type_decl *type_decl, beet_symbol_id name);

int beet_decl_registry_find_choice_variant_index(
    const beet_ast_type_decl *type_decl, const char *name, size_t name_len,
    size_t *out_index);
int beet_decl_registry_find_choice_variant_index_symbol(
    const beet_ast_type_decl *type_decl, beet_symbol_id name,
    size_t *out_index);

#endif
