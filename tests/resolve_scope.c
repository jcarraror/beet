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

int main(void) {
  test_bind_and_lookup();
  test_reject_duplicate_in_same_scope();
  test_shadow_in_nested_scope();
  test_lookup_missing();
  test_resolve_return_local_name();
  test_resolve_return_param_name();
  test_reject_missing_return_name();
  return 0;
}