#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "beet/parser/parser.h"
#include "beet/support/source.h"

static void test_simple_bind(void) {
  const char *text = "bind x = 10\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));

  assert(binding.is_mutable == 0);
  assert(binding.has_type == 0);
  assert(strncmp(binding.name, "x", 1) == 0);
  assert(strncmp(binding.value_text, "10", 2) == 0);
  assert(binding.expr.kind == BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(binding.expr.text, "10", 2) == 0);

  beet_source_file_dispose(&file);
}

static void test_mutable_bind(void) {
  const char *text = "mutable total = 0\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));

  assert(binding.is_mutable == 1);
  assert(binding.has_type == 0);
  assert(strncmp(binding.name, "total", 5) == 0);
  assert(binding.expr.kind == BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(binding.expr.text, "0", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_typed_bind(void) {
  const char *text = "bind x is Int = 5\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));

  assert(binding.has_type == 1);
  assert(strncmp(binding.type_name, "Int", 3) == 0);
  assert(binding.expr.kind == BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(binding.expr.text, "5", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_expression_bind(void) {
  const char *text = "bind total = 1 + 2 * 3\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));

  assert(binding.value_text == NULL);
  assert(binding.value_len == 0U);
  assert(binding.expr.kind == BEET_AST_EXPR_BINARY);
  assert(binding.expr.binary_op == BEET_AST_BINARY_ADD);
  assert(binding.expr.left != NULL);
  assert(binding.expr.left->kind == BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(binding.expr.left->text, "1", 1) == 0);
  assert(binding.expr.right != NULL);
  assert(binding.expr.right->kind == BEET_AST_EXPR_BINARY);
  assert(binding.expr.right->binary_op == BEET_AST_BINARY_MUL);
  assert(binding.expr.right->left != NULL);
  assert(strncmp(binding.expr.right->left->text, "2", 1) == 0);
  assert(binding.expr.right->right != NULL);
  assert(strncmp(binding.expr.right->right->text, "3", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_binding_spans(void) {
  const char *text = "bind x = 10\n";
  const char *value_text;
  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;

  value_text = strstr(text, "10");
  assert(value_text != NULL);

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));
  assert(binding.span.start.offset == 0U);
  assert(binding.span.end.offset == (size_t)((value_text - text) + 2));
  assert(binding.span.start.line == 1U);
  assert(binding.span.start.column == 1U);
  assert(binding.expr.span.start.offset == (size_t)(value_text - text));
  assert(binding.expr.span.end.offset == (size_t)((value_text - text) + 2));

  beet_source_file_dispose(&file);
}

int main(void) {
  test_simple_bind();
  test_mutable_bind();
  test_typed_bind();
  test_expression_bind();
  test_binding_spans();
  return 0;
}
