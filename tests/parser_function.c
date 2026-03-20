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
  assert(function_ast.has_trivial_return_const_int);
  assert(function_ast.trivial_return_const_int == 0);

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

  assert(function_ast.has_trivial_return_const_int);
  assert(function_ast.trivial_return_const_int == 7);

  beet_source_file_dispose(&file);
}

static void test_nested_block_function(void) {
  const char *text = "function abs(x is Int) returns Int {\n"
                     "    if x {\n"
                     "        return x\n"
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
  assert(function_ast.name_len == 3U);
  assert(strncmp(function_ast.name, "abs", 3) == 0);
  assert(function_ast.param_count == 1U);
  assert(!function_ast.has_trivial_return_const_int);

  beet_source_file_dispose(&file);
}

int main(void) {
  test_empty_function();
  test_function_with_params();
  test_nested_block_function();
  return 0;
}