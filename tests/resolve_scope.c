#include <assert.h>
#include <string.h>

#include "beet/parser/parser.h"
#include "beet/resolve/scope.h"
#include "beet/support/source.h"

static void test_bind_and_lookup(void) {
  beet_scope_stack stack;
  const beet_symbol *symbol;

  beet_scope_stack_init(&stack);

  assert(beet_scope_bind(&stack, "x", 0));
  assert(beet_scope_bind(&stack, "total", 1));

  symbol = beet_scope_lookup(&stack, "x");
  assert(symbol != NULL);
  assert(strcmp(symbol->name, "x") == 0);
  assert(symbol->depth == 0U);
  assert(symbol->is_mutable == 0);

  symbol = beet_scope_lookup(&stack, "total");
  assert(symbol != NULL);
  assert(strcmp(symbol->name, "total") == 0);
  assert(symbol->depth == 0U);
  assert(symbol->is_mutable == 1);
}

static void test_reject_duplicate_in_same_scope(void) {
  beet_scope_stack stack;

  beet_scope_stack_init(&stack);

  assert(beet_scope_bind(&stack, "x", 0));
  assert(!beet_scope_bind(&stack, "x", 1));
}

static void test_shadow_in_nested_scope(void) {
  beet_scope_stack stack;
  const beet_symbol *symbol;

  beet_scope_stack_init(&stack);

  assert(beet_scope_bind(&stack, "x", 0));
  assert(beet_scope_enter(&stack));
  assert(beet_scope_bind(&stack, "x", 1));

  symbol = beet_scope_lookup(&stack, "x");
  assert(symbol != NULL);
  assert(symbol->depth == 1U);
  assert(symbol->is_mutable == 1);

  assert(beet_scope_leave(&stack));

  symbol = beet_scope_lookup(&stack, "x");
  assert(symbol != NULL);
  assert(symbol->depth == 0U);
  assert(symbol->is_mutable == 0);
}

static void test_lookup_missing(void) {
  beet_scope_stack stack;

  beet_scope_stack_init(&stack);

  assert(beet_scope_lookup(&stack, "missing") == NULL);
}

static void test_resolve_return_local_name(void) {
  const char *text = "function main() returns Int {\n"
                     "    mutable total = 25\n"
                     "    return total\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));

  assert(function_ast.body[1].expr.kind == BEET_AST_EXPR_NAME);
  assert(function_ast.body[1].expr.is_resolved == 1);
  assert(function_ast.body[1].expr.resolved_depth == 0U);
  assert(function_ast.body[1].expr.resolved_is_mutable == 1);

  beet_source_file_dispose(&file);
}

static void test_resolve_return_param_name(void) {
  const char *text = "function identity(x is Int) returns Int {\n"
                     "    return x\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));

  assert(function_ast.body[0].expr.kind == BEET_AST_EXPR_NAME);
  assert(function_ast.body[0].expr.is_resolved == 1);
  assert(function_ast.body[0].expr.resolved_depth == 0U);
  assert(function_ast.body[0].expr.resolved_is_mutable == 0);

  beet_source_file_dispose(&file);
}

static void test_resolve_assignment_target_local(void) {
  const char *text = "function main() returns Int {\n"
                     "    mutable total = 1\n"
                     "    total = total + 2\n"
                     "    return total\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));

  assert(function_ast.body[1].kind == BEET_AST_STMT_ASSIGNMENT);
  assert(function_ast.body[1].assignment.is_resolved == 1);
  assert(function_ast.body[1].assignment.resolved_depth == 0U);
  assert(function_ast.body[1].assignment.resolved_is_mutable == 1);
  assert(function_ast.body[1].expr.left != NULL);
  assert(function_ast.body[1].expr.left->is_resolved == 1);
  assert(function_ast.body[1].expr.left->resolved_is_mutable == 1);

  beet_source_file_dispose(&file);
}

static void test_resolve_assignment_in_else_branch(void) {
  const char *text = "function main() returns Int {\n"
                     "    mutable total = 1\n"
                     "    if false {\n"
                     "        return 0\n"
                     "    } else {\n"
                     "        total = total + 2\n"
                     "    }\n"
                     "    return total\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));

  assert(function_ast.body[1].kind == BEET_AST_STMT_IF);
  assert(function_ast.body[1].else_body_count == 1U);
  assert(function_ast.body[1].else_body != NULL);
  assert(function_ast.body[1].else_body[0].kind == BEET_AST_STMT_ASSIGNMENT);
  assert(function_ast.body[1].else_body[0].assignment.is_resolved == 1);
  assert(function_ast.body[1].else_body[0].assignment.resolved_is_mutable == 1);

  beet_source_file_dispose(&file);
}

static void test_resolve_if_condition_bool_literal(void) {
  const char *text = "function main() returns Int {\n"
                     "    if true {\n"
                     "        return 1\n"
                     "    }\n"
                     "    return 0\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(function_ast.body[0].condition.kind == BEET_AST_EXPR_BOOL_LITERAL);
  assert(function_ast.body[0].condition.is_resolved == 0);

  beet_source_file_dispose(&file);
}

static void test_resolve_field_access_on_param(void) {
  const char *text = "function get(point is Point) returns Int {\n"
                     "    return point.x\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));

  assert(function_ast.body[0].expr.kind == BEET_AST_EXPR_FIELD);
  assert(function_ast.body[0].expr.left != NULL);
  assert(function_ast.body[0].expr.left->kind == BEET_AST_EXPR_NAME);
  assert(function_ast.body[0].expr.left->is_resolved == 1);
  assert(function_ast.body[0].expr.left->resolved_depth == 0U);
  assert(function_ast.body[0].expr.left->resolved_is_mutable == 0);

  beet_source_file_dispose(&file);
}

static void test_resolve_names_inside_structure_construction(void) {
  const char *text = "function main() returns Int {\n"
                     "    bind x = 1\n"
                     "    return Point(x = x, y = 2).x\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));

  assert(function_ast.body[1].expr.kind == BEET_AST_EXPR_FIELD);
  assert(function_ast.body[1].expr.left != NULL);
  assert(function_ast.body[1].expr.left->kind == BEET_AST_EXPR_CONSTRUCT);
  assert(function_ast.body[1].expr.left->field_init_count == 2U);
  assert(function_ast.body[1].expr.left->field_inits[0].value != NULL);
  assert(function_ast.body[1].expr.left->field_inits[0].value->kind ==
         BEET_AST_EXPR_NAME);
  assert(function_ast.body[1].expr.left->field_inits[0].value->is_resolved ==
         1);
  assert(function_ast.body[1].expr.left->field_inits[0].value->resolved_depth ==
         0U);

  beet_source_file_dispose(&file);
}

static void test_resolve_binding_expression_uses_prior_local(void) {
  const char *text = "function main() returns Int {\n"
                     "    bind x = 1\n"
                     "    bind y = x + 2\n"
                     "    return y\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));

  assert(function_ast.body[1].kind == BEET_AST_STMT_BINDING);
  assert(function_ast.body[1].binding.expr.kind == BEET_AST_EXPR_BINARY);
  assert(function_ast.body[1].binding.expr.left != NULL);
  assert(function_ast.body[1].binding.expr.left->kind == BEET_AST_EXPR_NAME);
  assert(function_ast.body[1].binding.expr.left->is_resolved == 1);
  assert(function_ast.body[1].binding.expr.left->resolved_depth == 0U);
  assert(function_ast.body[2].expr.kind == BEET_AST_EXPR_NAME);
  assert(function_ast.body[2].expr.is_resolved == 1);

  beet_source_file_dispose(&file);
}

static void test_resolve_function_call_against_decl_set(void) {
  const char *text = "function add(x is Int, y is Int) returns Int {\n"
                     "    return x + y\n"
                     "}\n"
                     "function main() returns Int {\n"
                     "    bind value = 2\n"
                     "    return add(value, 3)\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function functions[2];

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &functions[0]));
  assert(beet_parser_parse_function(&parser, &functions[1]));
  assert(beet_resolve_function_with_decls(&functions[1], functions, 2U));

  assert(functions[1].body[1].expr.kind == BEET_AST_EXPR_CALL);
  assert(functions[1].body[1].expr.call_is_resolved == 1);
  assert(functions[1].body[1].expr.call_target_index == 0U);
  assert(functions[1].body[1].expr.arg_count == 2U);
  assert(functions[1].body[1].expr.args[0] != NULL);
  assert(functions[1].body[1].expr.args[0]->kind == BEET_AST_EXPR_NAME);
  assert(functions[1].body[1].expr.args[0]->is_resolved == 1);
  assert(functions[1].body[1].expr.args[0]->resolved_depth == 0U);

  beet_source_file_dispose(&file);
}

static void test_reject_missing_called_function(void) {
  const char *text = "function main() returns Int {\n"
                     "    return missing(1)\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(!beet_resolve_function_with_decls(&function_ast, &function_ast, 1U));

  beet_source_file_dispose(&file);
}

static void test_reject_missing_return_name(void) {
  const char *text = "function main() returns Int {\n"
                     "    return missing\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(!beet_resolve_function(&function_ast));

  beet_source_file_dispose(&file);
}

static void test_reject_missing_assignment_target(void) {
  const char *text = "function main() returns Int {\n"
                     "    missing = 1\n"
                     "    return 0\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(!beet_resolve_function(&function_ast));

  beet_source_file_dispose(&file);
}

static void test_resolve_name_inside_choice_construction(void) {
  const char *text = "function main() returns Option {\n"
                     "    bind value = 1\n"
                     "    return Option.some(value)\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(function_ast.body[1].expr.kind == BEET_AST_EXPR_CONSTRUCT);
  assert(function_ast.body[1].expr.field_init_count == 1U);
  assert(function_ast.body[1].expr.field_inits[0].value != NULL);
  assert(function_ast.body[1].expr.field_inits[0].value->kind ==
         BEET_AST_EXPR_NAME);
  assert(function_ast.body[1].expr.field_inits[0].value->is_resolved == 1);
  assert(function_ast.body[1].expr.field_inits[0].value->resolved_depth == 0U);

  beet_source_file_dispose(&file);
}

int main(void) {
  test_bind_and_lookup();
  test_reject_duplicate_in_same_scope();
  test_shadow_in_nested_scope();
  test_lookup_missing();
  test_resolve_return_local_name();
  test_resolve_return_param_name();
  test_resolve_assignment_target_local();
  test_resolve_assignment_in_else_branch();
  test_resolve_field_access_on_param();
  test_resolve_names_inside_structure_construction();
  test_resolve_binding_expression_uses_prior_local();
  test_resolve_if_condition_bool_literal();
  test_resolve_function_call_against_decl_set();
  test_resolve_name_inside_choice_construction();
  test_reject_missing_return_name();
  test_reject_missing_called_function();
  test_reject_missing_assignment_target();
  return 0;
}
