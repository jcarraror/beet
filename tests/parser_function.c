#include <assert.h>
#include <string.h>

#include "beet/parser/parser.h"
#include "beet/support/source.h"

static void test_empty_function(void) {
  const char *text = "function main() returns Int {\n"
                     "    return 0\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(function_ast.name_len == 4U);
  assert(strncmp(function_ast.name, "main", 4) == 0);
  assert(function_ast.return_type_name_len == 3U);
  assert(strncmp(function_ast.return_type_name, "Int", 3) == 0);
  assert(function_ast.param_count == 0U);
  assert(function_ast.body_count == 1U);
  assert(function_ast.body[0].kind == BEET_AST_STMT_RETURN);
  assert(function_ast.body[0].expr.kind == BEET_AST_EXPR_INT_LITERAL);
  assert(function_ast.body[0].expr.text_len == 1U);
  assert(strncmp(function_ast.body[0].expr.text, "0", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_function_with_params(void) {
  const char *text =
      "function length(point is Point, scale is Int) returns Int {\n"
      "    return 7\n"
      "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(function_ast.name_len == 6U);
  assert(strncmp(function_ast.name, "length", 6) == 0);
  assert(function_ast.return_type_name_len == 3U);
  assert(strncmp(function_ast.return_type_name, "Int", 3) == 0);
  assert(function_ast.param_count == 2U);

  assert(strncmp(function_ast.params[0].name, "point", 5) == 0);
  assert(strncmp(function_ast.params[0].type_name, "Point", 5) == 0);

  assert(strncmp(function_ast.params[1].name, "scale", 5) == 0);
  assert(strncmp(function_ast.params[1].type_name, "Int", 3) == 0);

  assert(function_ast.body_count == 1U);
  assert(function_ast.body[0].kind == BEET_AST_STMT_RETURN);
  assert(function_ast.body[0].expr.kind == BEET_AST_EXPR_INT_LITERAL);
  assert(function_ast.body[0].expr.text_len == 1U);
  assert(strncmp(function_ast.body[0].expr.text, "7", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_name_return_function(void) {
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
  assert(function_ast.body_count == 1U);
  assert(function_ast.body[0].kind == BEET_AST_STMT_RETURN);
  assert(function_ast.body[0].expr.kind == BEET_AST_EXPR_NAME);
  assert(function_ast.body[0].expr.text_len == 1U);
  assert(strncmp(function_ast.body[0].expr.text, "x", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_function_body_bindings(void) {
  const char *text = "function main() returns Int {\n"
                     "    bind x = 10\n"
                     "    mutable total is Int = 25\n"
                     "    return total\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(function_ast.body_count == 3U);

  assert(function_ast.body[0].kind == BEET_AST_STMT_BINDING);
  assert(function_ast.body[0].binding.is_mutable == 0);
  assert(function_ast.body[0].binding.has_type == 0);
  assert(function_ast.body[0].binding.name_len == 1U);
  assert(strncmp(function_ast.body[0].binding.name, "x", 1) == 0);
  assert(function_ast.body[0].binding.value_len == 2U);
  assert(strncmp(function_ast.body[0].binding.value_text, "10", 2) == 0);
  assert(function_ast.body[0].binding.expr.kind == BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(function_ast.body[0].binding.expr.text, "10", 2) == 0);

  assert(function_ast.body[1].kind == BEET_AST_STMT_BINDING);
  assert(function_ast.body[1].binding.is_mutable == 1);
  assert(function_ast.body[1].binding.has_type == 1);
  assert(function_ast.body[1].binding.name_len == 5U);
  assert(strncmp(function_ast.body[1].binding.name, "total", 5) == 0);
  assert(function_ast.body[1].binding.type_name_len == 3U);
  assert(strncmp(function_ast.body[1].binding.type_name, "Int", 3) == 0);
  assert(function_ast.body[1].binding.value_len == 2U);
  assert(strncmp(function_ast.body[1].binding.value_text, "25", 2) == 0);
  assert(function_ast.body[1].binding.expr.kind == BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(function_ast.body[1].binding.expr.text, "25", 2) == 0);

  assert(function_ast.body[2].kind == BEET_AST_STMT_RETURN);
  assert(function_ast.body[2].expr.kind == BEET_AST_EXPR_NAME);
  assert(function_ast.body[2].expr.text_len == 5U);
  assert(strncmp(function_ast.body[2].expr.text, "total", 5) == 0);

  beet_source_file_dispose(&file);
}

static void test_function_body_binding_expression(void) {
  const char *text = "function main() returns Int {\n"
                     "    bind total = 1 + 2 * 3\n"
                     "    return total\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(function_ast.body_count == 2U);
  assert(function_ast.body[0].kind == BEET_AST_STMT_BINDING);
  assert(function_ast.body[0].binding.value_text == NULL);
  assert(function_ast.body[0].binding.value_len == 0U);
  assert(function_ast.body[0].binding.expr.kind == BEET_AST_EXPR_BINARY);
  assert(function_ast.body[0].binding.expr.binary_op == BEET_AST_BINARY_ADD);
  assert(function_ast.body[0].binding.expr.left != NULL);
  assert(function_ast.body[0].binding.expr.left->kind ==
         BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(function_ast.body[0].binding.expr.left->text, "1", 1) == 0);
  assert(function_ast.body[0].binding.expr.right != NULL);
  assert(function_ast.body[0].binding.expr.right->kind == BEET_AST_EXPR_BINARY);
  assert(function_ast.body[0].binding.expr.right->binary_op ==
         BEET_AST_BINARY_MUL);

  beet_source_file_dispose(&file);
}

static void test_assignment_statement_function(void) {
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
  assert(function_ast.body_count == 3U);
  assert(function_ast.body[0].kind == BEET_AST_STMT_BINDING);
  assert(function_ast.body[1].kind == BEET_AST_STMT_ASSIGNMENT);
  assert(function_ast.body[1].assignment.name_len == 5U);
  assert(strncmp(function_ast.body[1].assignment.name, "total", 5) == 0);
  assert(function_ast.body[1].expr.kind == BEET_AST_EXPR_BINARY);
  assert(function_ast.body[1].expr.binary_op == BEET_AST_BINARY_ADD);
  assert(function_ast.body[1].expr.left != NULL);
  assert(function_ast.body[1].expr.left->kind == BEET_AST_EXPR_NAME);
  assert(strncmp(function_ast.body[1].expr.left->text, "total", 5) == 0);
  assert(function_ast.body[1].expr.right != NULL);
  assert(function_ast.body[1].expr.right->kind == BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(function_ast.body[1].expr.right->text, "2", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_expression_precedence_function(void) {
  const char *text = "function main() returns Int {\n"
                     "    return 1 + 2 * 3\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(function_ast.body_count == 1U);
  assert(function_ast.body[0].expr.kind == BEET_AST_EXPR_BINARY);
  assert(function_ast.body[0].expr.binary_op == BEET_AST_BINARY_ADD);
  assert(function_ast.body[0].expr.left != NULL);
  assert(function_ast.body[0].expr.left->kind == BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(function_ast.body[0].expr.left->text, "1", 1) == 0);
  assert(function_ast.body[0].expr.right != NULL);
  assert(function_ast.body[0].expr.right->kind == BEET_AST_EXPR_BINARY);
  assert(function_ast.body[0].expr.right->binary_op == BEET_AST_BINARY_MUL);
  assert(function_ast.body[0].expr.right->left != NULL);
  assert(strncmp(function_ast.body[0].expr.right->left->text, "2", 1) == 0);
  assert(function_ast.body[0].expr.right->right != NULL);
  assert(strncmp(function_ast.body[0].expr.right->right->text, "3", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_unary_and_grouped_expression_function(void) {
  const char *text = "function main() returns Int {\n"
                     "    return -(1 + 2)\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(function_ast.body_count == 1U);
  assert(function_ast.body[0].expr.kind == BEET_AST_EXPR_UNARY);
  assert(function_ast.body[0].expr.unary_op == BEET_AST_UNARY_NEGATE);
  assert(function_ast.body[0].expr.left != NULL);
  assert(function_ast.body[0].expr.left->kind == BEET_AST_EXPR_BINARY);
  assert(function_ast.body[0].expr.left->binary_op == BEET_AST_BINARY_ADD);
  assert(function_ast.body[0].expr.left->left != NULL);
  assert(strncmp(function_ast.body[0].expr.left->left->text, "1", 1) == 0);
  assert(function_ast.body[0].expr.left->right != NULL);
  assert(strncmp(function_ast.body[0].expr.left->right->text, "2", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_comparison_expression_function(void) {
  const char *text = "function main() returns Bool {\n"
                     "    return 1 + 2 < 3 * 4\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(function_ast.body_count == 1U);
  assert(function_ast.body[0].expr.kind == BEET_AST_EXPR_BINARY);
  assert(function_ast.body[0].expr.binary_op == BEET_AST_BINARY_LT);
  assert(function_ast.body[0].expr.left != NULL);
  assert(function_ast.body[0].expr.left->kind == BEET_AST_EXPR_BINARY);
  assert(function_ast.body[0].expr.left->binary_op == BEET_AST_BINARY_ADD);
  assert(function_ast.body[0].expr.right != NULL);
  assert(function_ast.body[0].expr.right->kind == BEET_AST_EXPR_BINARY);
  assert(function_ast.body[0].expr.right->binary_op == BEET_AST_BINARY_MUL);

  beet_source_file_dispose(&file);
}

static void test_assignment_statement_inside_if_function(void) {
  const char *text = "function main() returns Int {\n"
                     "    mutable x = 1\n"
                     "    if true {\n"
                     "        x = x + 1\n"
                     "    }\n"
                     "    return x\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(function_ast.body_count == 3U);
  assert(function_ast.body[1].kind == BEET_AST_STMT_IF);
  assert(function_ast.body[1].then_body_count == 1U);
  assert(function_ast.body[1].then_body != NULL);
  assert(function_ast.body[1].then_body[0].kind == BEET_AST_STMT_ASSIGNMENT);
  assert(function_ast.body[1].then_body[0].assignment.name_len == 1U);
  assert(strncmp(function_ast.body[1].then_body[0].assignment.name, "x", 1) ==
         0);
  assert(function_ast.body[1].then_body[0].expr.kind == BEET_AST_EXPR_BINARY);

  beet_source_file_dispose(&file);
}

static void test_while_statement_function(void) {
  const char *text = "function main() returns Int {\n"
                     "    while true {\n"
                     "        bind x = 1\n"
                     "        if false {\n"
                     "            return x\n"
                     "        }\n"
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
  assert(function_ast.body_count == 2U);
  assert(function_ast.body[0].kind == BEET_AST_STMT_WHILE);
  assert(function_ast.body[0].condition.kind == BEET_AST_EXPR_BOOL_LITERAL);
  assert(function_ast.body[0].condition.text_len == 4U);
  assert(strncmp(function_ast.body[0].condition.text, "true", 4) == 0);
  assert(function_ast.body[0].loop_body_count == 2U);
  assert(function_ast.body[0].loop_body != NULL);

  assert(function_ast.body[0].loop_body[0].kind == BEET_AST_STMT_BINDING);
  assert(function_ast.body[0].loop_body[0].binding.name_len == 1U);
  assert(strncmp(function_ast.body[0].loop_body[0].binding.name, "x", 1) == 0);

  assert(function_ast.body[0].loop_body[1].kind == BEET_AST_STMT_IF);
  assert(function_ast.body[0].loop_body[1].condition.kind ==
         BEET_AST_EXPR_BOOL_LITERAL);
  assert(function_ast.body[0].loop_body[1].condition.text_len == 5U);
  assert(strncmp(function_ast.body[0].loop_body[1].condition.text, "false",
                 5) == 0);
  assert(function_ast.body[0].loop_body[1].then_body_count == 1U);
  assert(function_ast.body[0].loop_body[1].then_body != NULL);
  assert(function_ast.body[0].loop_body[1].then_body[0].kind ==
         BEET_AST_STMT_RETURN);
  assert(function_ast.body[0].loop_body[1].then_body[0].expr.kind ==
         BEET_AST_EXPR_NAME);
  assert(function_ast.body[0].loop_body[1].then_body[0].expr.text_len == 1U);
  assert(strncmp(function_ast.body[0].loop_body[1].then_body[0].expr.text, "x",
                 1) == 0);

  assert(function_ast.body[1].kind == BEET_AST_STMT_RETURN);
  assert(function_ast.body[1].expr.kind == BEET_AST_EXPR_INT_LITERAL);
  assert(function_ast.body[1].expr.text_len == 1U);
  assert(strncmp(function_ast.body[1].expr.text, "0", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_if_else_statement_function(void) {
  const char *text = "function choose(flag is Bool) returns Int {\n"
                     "    if flag {\n"
                     "        return 1\n"
                     "    } else {\n"
                     "        return 2\n"
                     "    }\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(function_ast.body_count == 1U);
  assert(function_ast.body[0].kind == BEET_AST_STMT_IF);
  assert(function_ast.body[0].condition.kind == BEET_AST_EXPR_NAME);
  assert(function_ast.body[0].then_body_count == 1U);
  assert(function_ast.body[0].then_body != NULL);
  assert(function_ast.body[0].then_body[0].kind == BEET_AST_STMT_RETURN);
  assert(function_ast.body[0].else_body_count == 1U);
  assert(function_ast.body[0].else_body != NULL);
  assert(function_ast.body[0].else_body[0].kind == BEET_AST_STMT_RETURN);
  assert(function_ast.body[0].else_body[0].expr.kind ==
         BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(function_ast.body[0].else_body[0].expr.text, "2", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_if_statement_function(void) {
  const char *text = "function choose(x is Int, y is Int) returns Int {\n"
                     "    if x {\n"
                     "        bind z = 1\n"
                     "        if y {\n"
                     "            return z\n"
                     "        }\n"
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
  assert(function_ast.body_count == 2U);
  assert(function_ast.body[0].kind == BEET_AST_STMT_IF);
  assert(function_ast.body[0].condition.kind == BEET_AST_EXPR_NAME);
  assert(function_ast.body[0].condition.text_len == 1U);
  assert(strncmp(function_ast.body[0].condition.text, "x", 1) == 0);
  assert(function_ast.body[0].then_body_count == 2U);
  assert(function_ast.body[0].then_body != NULL);

  assert(function_ast.body[0].then_body[0].kind == BEET_AST_STMT_BINDING);
  assert(function_ast.body[0].then_body[0].binding.name_len == 1U);
  assert(strncmp(function_ast.body[0].then_body[0].binding.name, "z", 1) == 0);

  assert(function_ast.body[0].then_body[1].kind == BEET_AST_STMT_IF);
  assert(function_ast.body[0].then_body[1].condition.kind ==
         BEET_AST_EXPR_NAME);
  assert(function_ast.body[0].then_body[1].condition.text_len == 1U);
  assert(strncmp(function_ast.body[0].then_body[1].condition.text, "y", 1) ==
         0);
  assert(function_ast.body[0].then_body[1].then_body_count == 1U);
  assert(function_ast.body[0].then_body[1].then_body != NULL);
  assert(function_ast.body[0].then_body[1].then_body[0].kind ==
         BEET_AST_STMT_RETURN);
  assert(function_ast.body[0].then_body[1].then_body[0].expr.kind ==
         BEET_AST_EXPR_NAME);
  assert(function_ast.body[0].then_body[1].then_body[0].expr.text_len == 1U);
  assert(strncmp(function_ast.body[0].then_body[1].then_body[0].expr.text, "z",
                 1) == 0);

  assert(function_ast.body[1].kind == BEET_AST_STMT_RETURN);
  assert(function_ast.body[1].expr.kind == BEET_AST_EXPR_INT_LITERAL);
  assert(function_ast.body[1].expr.text_len == 1U);
  assert(strncmp(function_ast.body[1].expr.text, "0", 1) == 0);

  beet_source_file_dispose(&file);
}

static void test_function_call_expression_function(void) {
  const char *text = "function main() returns Int {\n"
                     "    return add(1, 2 * 3)\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(function_ast.body_count == 1U);
  assert(function_ast.body[0].kind == BEET_AST_STMT_RETURN);
  assert(function_ast.body[0].expr.kind == BEET_AST_EXPR_CALL);
  assert(function_ast.body[0].expr.text_len == 3U);
  assert(strncmp(function_ast.body[0].expr.text, "add", 3) == 0);
  assert(function_ast.body[0].expr.arg_count == 2U);
  assert(function_ast.body[0].expr.args[0] != NULL);
  assert(function_ast.body[0].expr.args[0]->kind == BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(function_ast.body[0].expr.args[0]->text, "1", 1) == 0);
  assert(function_ast.body[0].expr.args[1] != NULL);
  assert(function_ast.body[0].expr.args[1]->kind == BEET_AST_EXPR_BINARY);
  assert(function_ast.body[0].expr.args[1]->binary_op == BEET_AST_BINARY_MUL);

  beet_source_file_dispose(&file);
}

static void test_structure_construction_and_field_access_function(void) {
  const char *text = "function main() returns Int {\n"
                     "    return Point(x = 3, y = 4).x\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(function_ast.body_count == 1U);
  assert(function_ast.body[0].kind == BEET_AST_STMT_RETURN);
  assert(function_ast.body[0].expr.kind == BEET_AST_EXPR_FIELD);
  assert(function_ast.body[0].expr.left != NULL);
  assert(function_ast.body[0].expr.text_len == 1U);
  assert(strncmp(function_ast.body[0].expr.text, "x", 1) == 0);

  assert(function_ast.body[0].expr.left->kind == BEET_AST_EXPR_CONSTRUCT);
  assert(function_ast.body[0].expr.left->text_len == 5U);
  assert(strncmp(function_ast.body[0].expr.left->text, "Point", 5) == 0);
  assert(function_ast.body[0].expr.left->field_init_count == 2U);

  assert(strncmp(function_ast.body[0].expr.left->field_inits[0].name, "x", 1) ==
         0);
  assert(function_ast.body[0].expr.left->field_inits[0].value != NULL);
  assert(function_ast.body[0].expr.left->field_inits[0].value->kind ==
         BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(function_ast.body[0].expr.left->field_inits[0].value->text,
                 "3", 1) == 0);

  assert(strncmp(function_ast.body[0].expr.left->field_inits[1].name, "y", 1) ==
         0);
  assert(function_ast.body[0].expr.left->field_inits[1].value != NULL);
  assert(function_ast.body[0].expr.left->field_inits[1].value->kind ==
         BEET_AST_EXPR_INT_LITERAL);
  assert(strncmp(function_ast.body[0].expr.left->field_inits[1].value->text,
                 "4", 1) == 0);

  beet_source_file_dispose(&file);
}

int main(void) {
  test_empty_function();
  test_function_with_params();
  test_name_return_function();
  test_function_body_bindings();
  test_function_body_binding_expression();
  test_assignment_statement_function();
  test_expression_precedence_function();
  test_unary_and_grouped_expression_function();
  test_comparison_expression_function();
  test_assignment_statement_inside_if_function();
  test_while_statement_function();
  test_if_else_statement_function();
  test_if_statement_function();
  test_function_call_expression_function();
  test_structure_construction_and_field_access_function();
  return 0;
}
