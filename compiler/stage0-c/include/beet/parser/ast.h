#ifndef BEET_PARSER_AST_H
#define BEET_PARSER_AST_H

#include <stddef.h>

#define BEET_AST_MAX_PARAMS 16
#define BEET_AST_MAX_FIELDS 32
#define BEET_AST_MAX_BODY_STMTS 64

typedef enum beet_ast_expr_kind {
  BEET_AST_EXPR_INVALID = 0,
  BEET_AST_EXPR_INT_LITERAL,
  BEET_AST_EXPR_NAME
} beet_ast_expr_kind;

typedef struct beet_ast_expr {
  beet_ast_expr_kind kind;
  const char *text;
  size_t text_len;

  int is_resolved;
  size_t resolved_depth;
  int resolved_is_mutable;
} beet_ast_expr;

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

typedef enum beet_ast_stmt_kind {
  BEET_AST_STMT_INVALID = 0,
  BEET_AST_STMT_BINDING,
  BEET_AST_STMT_RETURN
} beet_ast_stmt_kind;

typedef struct beet_ast_stmt {
  beet_ast_stmt_kind kind;
  beet_ast_binding binding;
  beet_ast_expr expr;
} beet_ast_stmt;

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

  beet_ast_stmt body[BEET_AST_MAX_BODY_STMTS];
  size_t body_count;
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