#include <assert.h>
#include <string.h>

#include "beet/parser/parser.h"
#include "beet/support/source.h"

static void test_parse_module_with_types_and_functions(void) {
  const char *text = "module core\n"
                     "\n"
                     "type Point = structure {\n"
                     "    x is Int\n"
                     "    y is Int\n"
                     "}\n"
                     "\n"
                     "function helper(x is Int) returns Int {\n"
                     "    return x\n"
                     "}\n"
                     "\n"
                     "function main() returns Int {\n"
                     "    return helper(7)\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_module module;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_module(&parser, &module));
  assert(strncmp(module.name, "core", 4U) == 0);
  assert(module.type_decl_count == 1U);
  assert(module.function_count == 2U);
  assert(strncmp(module.type_decls[0].name, "Point", 5U) == 0);
  assert(strncmp(module.functions[0].name, "helper", 6U) == 0);
  assert(strncmp(module.functions[1].name, "main", 4U) == 0);
  assert(module.functions[1].body_count == 1U);
  assert(module.functions[1].body[0].kind == BEET_AST_STMT_RETURN);
  assert(module.functions[1].body[0].expr.kind == BEET_AST_EXPR_CALL);

  beet_source_file_dispose(&file);
}

static void test_module_span_covers_header_and_last_decl(void) {
  const char *text = "module sample\n"
                     "function main() returns Int {\n"
                     "    return 7\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_module module;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_module(&parser, &module));
  assert(module.span.start.offset == 0U);
  assert(module.span.end.offset == strlen(text) - 1U);

  beet_source_file_dispose(&file);
}

int main(void) {
  test_parse_module_with_types_and_functions();
  test_module_span_covers_header_and_last_decl();
  return 0;
}
