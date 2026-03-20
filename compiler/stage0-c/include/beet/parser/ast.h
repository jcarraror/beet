#ifndef BEET_PARSER_AST_H
#define BEET_PARSER_AST_H

#include <stddef.h>

#define BEET_AST_MAX_PARAMS 16
#define BEET_AST_MAX_FIELDS 32

typedef struct beet_ast_binding {
  const char *name;
  size_t name_len;

  int is_mutable;
  int has_type;

  const char *type_name;
  size_t type_name_len;

  const char *value_text;
  size_t value_len;
} beet_ast_binding;

typedef struct beet_ast_param {
  const char *name;
  size_t name_len;

  const char *type_name;
  size_t type_name_len;
} beet_ast_param;

typedef struct beet_ast_function {
  const char *name;
  size_t name_len;

  const char *return_type_name;
  size_t return_type_name_len;

  beet_ast_param params[BEET_AST_MAX_PARAMS];
  size_t param_count;

  int has_trivial_return_const_int;
  int trivial_return_const_int;
} beet_ast_function;

typedef struct beet_ast_field {
  const char *name;
  size_t name_len;

  const char *type_name;
  size_t type_name_len;
} beet_ast_field;

typedef struct beet_ast_type_decl {
  const char *name;
  size_t name_len;

  int is_structure;

  beet_ast_field fields[BEET_AST_MAX_FIELDS];
  size_t field_count;
} beet_ast_type_decl;

#endif