#ifndef BEET_PARSER_AST_H
#define BEET_PARSER_AST_H

#include <stddef.h>

#include "beet/support/source.h"

#define BEET_AST_MAX_PARAMS 16
#define BEET_AST_MAX_FIELDS 32
#define BEET_AST_MAX_VARIANTS 32
#define BEET_AST_MAX_BODY_STMTS 64
#define BEET_AST_MAX_STMT_NODES 128
#define BEET_AST_MAX_EXPR_NODES 128

typedef enum beet_ast_expr_kind {
  BEET_AST_EXPR_INVALID = 0,
  BEET_AST_EXPR_INT_LITERAL,
  BEET_AST_EXPR_BOOL_LITERAL,
  BEET_AST_EXPR_NAME,
  BEET_AST_EXPR_CALL,
  BEET_AST_EXPR_CONSTRUCT,
  BEET_AST_EXPR_FIELD,
  BEET_AST_EXPR_UNARY,
  BEET_AST_EXPR_BINARY
} beet_ast_expr_kind;

typedef enum beet_ast_unary_op { BEET_AST_UNARY_NEGATE = 0 } beet_ast_unary_op;

typedef enum beet_ast_binary_op {
  BEET_AST_BINARY_ADD = 0,
  BEET_AST_BINARY_SUB,
  BEET_AST_BINARY_MUL,
  BEET_AST_BINARY_DIV,
  BEET_AST_BINARY_EQ,
  BEET_AST_BINARY_NE,
  BEET_AST_BINARY_LT,
  BEET_AST_BINARY_LE,
  BEET_AST_BINARY_GT,
  BEET_AST_BINARY_GE
} beet_ast_binary_op;

struct beet_ast_expr;
struct beet_ast_type_decl;
struct beet_ast_field;
struct beet_ast_choice_variant;

typedef struct beet_ast_field_init {
  const char *name;
  size_t name_len;
  struct beet_ast_expr *value;
} beet_ast_field_init;

typedef struct beet_ast_expr {
  beet_ast_expr_kind kind;
  const char *text;
  size_t text_len;
  beet_source_span span;

  int is_resolved;
  size_t resolved_depth;
  int resolved_is_mutable;
  int call_is_resolved;
  size_t call_target_index;
  const struct beet_ast_type_decl *resolved_type_decl;
  const struct beet_ast_field *resolved_field_decl;
  const struct beet_ast_choice_variant *resolved_variant_decl;
  size_t resolved_variant_index;

  beet_ast_unary_op unary_op;
  beet_ast_binary_op binary_op;
  struct beet_ast_expr *left;
  struct beet_ast_expr *right;

  struct beet_ast_expr *args[BEET_AST_MAX_PARAMS];
  size_t arg_count;

  beet_ast_field_init field_inits[BEET_AST_MAX_FIELDS];
  size_t field_init_count;
} beet_ast_expr;

typedef struct beet_ast_binding {
  const char *name;
  size_t name_len;
  beet_source_span span;

  int is_mutable;
  int has_type;

  const char *type_name;
  size_t type_name_len;
  const struct beet_ast_type_decl *resolved_type_decl;

  const char *value_text;
  size_t value_len;

  beet_ast_expr expr;
} beet_ast_binding;

typedef struct beet_ast_assignment {
  const char *name;
  size_t name_len;
  beet_source_span span;

  int is_resolved;
  size_t resolved_depth;
  int resolved_is_mutable;
} beet_ast_assignment;

typedef enum beet_ast_stmt_kind {
  BEET_AST_STMT_INVALID = 0,
  BEET_AST_STMT_BINDING,
  BEET_AST_STMT_ASSIGNMENT,
  BEET_AST_STMT_RETURN,
  BEET_AST_STMT_IF,
  BEET_AST_STMT_WHILE,
  BEET_AST_STMT_MATCH
} beet_ast_stmt_kind;

typedef struct beet_ast_match_case {
  const char *variant_name;
  size_t variant_name_len;
  beet_source_span span;

  int binds_payload;
  const struct beet_ast_choice_variant *resolved_variant_decl;
  size_t resolved_variant_index;
  const char *binding_name;
  size_t binding_name_len;

  struct beet_ast_stmt *body;
  size_t body_count;
} beet_ast_match_case;

typedef struct beet_ast_stmt {
  beet_ast_stmt_kind kind;
  beet_source_span span;
  beet_ast_binding binding;
  beet_ast_assignment assignment;
  beet_ast_expr expr;
  beet_ast_expr condition;
  beet_ast_expr match_expr;
  beet_ast_match_case match_cases[BEET_AST_MAX_VARIANTS];
  size_t match_case_count;
  struct beet_ast_stmt *then_body;
  size_t then_body_count;
  struct beet_ast_stmt *else_body;
  size_t else_body_count;
  struct beet_ast_stmt *loop_body;
  size_t loop_body_count;
} beet_ast_stmt;

typedef struct beet_ast_param {
  const char *name;
  size_t name_len;
  beet_source_span span;

  const char *type_name;
  size_t type_name_len;
  const struct beet_ast_type_decl *resolved_type_decl;
} beet_ast_param;

typedef struct beet_ast_function {
  const char *name;
  size_t name_len;
  beet_source_span span;

  const char *return_type_name;
  size_t return_type_name_len;

  beet_ast_param params[BEET_AST_MAX_PARAMS];
  size_t param_count;

  beet_ast_stmt body[BEET_AST_MAX_BODY_STMTS];
  size_t body_count;

  beet_ast_stmt stmt_nodes[BEET_AST_MAX_STMT_NODES];
  size_t stmt_node_count;

  beet_ast_expr expr_nodes[BEET_AST_MAX_EXPR_NODES];
  size_t expr_count;
} beet_ast_function;

typedef struct beet_ast_field {
  const char *name;
  size_t name_len;
  beet_source_span span;

  const char *type_name;
  size_t type_name_len;
} beet_ast_field;

typedef struct beet_ast_choice_variant {
  const char *name;
  size_t name_len;
  beet_source_span span;

  int has_payload;
  const char *payload_type_name;
  size_t payload_type_name_len;
} beet_ast_choice_variant;

typedef struct beet_ast_type_param {
  const char *name;
  size_t name_len;
  beet_source_span span;
} beet_ast_type_param;

typedef struct beet_ast_type_decl {
  const char *name;
  size_t name_len;
  beet_source_span span;

  beet_ast_type_param params[BEET_AST_MAX_PARAMS];
  size_t param_count;

  int is_structure;
  int is_choice;

  beet_ast_field fields[BEET_AST_MAX_FIELDS];
  size_t field_count;

  beet_ast_choice_variant variants[BEET_AST_MAX_VARIANTS];
  size_t variant_count;
} beet_ast_type_decl;

#endif
