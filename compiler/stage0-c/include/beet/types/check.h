#ifndef BEET_TYPES_CHECK_H
#define BEET_TYPES_CHECK_H

#include "beet/parser/ast.h"
#include "beet/semantics/registry.h"
#include "beet/types/types.h"

typedef struct beet_type_check_result {
  int ok;
  beet_type declared_type;
  beet_type value_type;
} beet_type_check_result;

beet_type beet_type_check_binding(const beet_ast_binding *binding);
beet_type_check_result
beet_type_check_binding_annotation(const beet_ast_binding *binding);
int beet_type_check_function_signature_with_registry(
    const beet_ast_function *function_ast, const beet_decl_registry *registry);
int beet_type_check_function_signature_with_type_decls(
    const beet_ast_function *function_ast, const beet_ast_type_decl *type_decls,
    size_t decl_count);
int beet_type_check_function_signature(const beet_ast_function *function_ast);
int beet_type_check_function_body_with_registry(
    const beet_ast_function *function_ast, const beet_decl_registry *registry);
int beet_type_check_function_body_with_decls(
    const beet_ast_function *function_ast, const beet_ast_type_decl *type_decls,
    size_t decl_count, const beet_ast_function *function_decls,
    size_t function_count);
int beet_type_check_function_body_with_type_decls(
    const beet_ast_function *function_ast, const beet_ast_type_decl *type_decls,
    size_t decl_count);
int beet_type_check_function_body(const beet_ast_function *function_ast);
int beet_type_check_type_decls_with_registry(
    const beet_decl_registry *registry);
int beet_type_check_type_decl(const beet_ast_type_decl *type_decl);
int beet_type_check_type_decls(const beet_ast_type_decl *type_decls,
                               size_t decl_count);

#endif
