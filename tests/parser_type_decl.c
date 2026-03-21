#include <assert.h>
#include <string.h>

#include "beet/parser/parser.h"
#include "beet/support/source.h"

static void test_simple_structure(void) {
  const char *text = "type Point = structure {\n"
                     "    x is Int\n"
                     "    y is Int\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_type_decl type_decl;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_type_decl(&parser, &type_decl));
  assert(strncmp(type_decl.name, "Point", 5) == 0);
  assert(type_decl.is_structure == 1);
  assert(type_decl.field_count == 2U);

  assert(strncmp(type_decl.fields[0].name, "x", 1) == 0);
  assert(strncmp(type_decl.fields[0].type_name, "Int", 3) == 0);
  assert(strncmp(type_decl.fields[1].name, "y", 1) == 0);
  assert(strncmp(type_decl.fields[1].type_name, "Int", 3) == 0);

  beet_source_file_dispose(&file);
}

static void test_parameterized_structure(void) {
  const char *text = "type Box(Value) = structure {\n"
                     "    value is Value\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_type_decl type_decl;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_type_decl(&parser, &type_decl));
  assert(strncmp(type_decl.name, "Box", 3) == 0);
  assert(type_decl.is_structure == 1);
  assert(type_decl.param_count == 1U);
  assert(strncmp(type_decl.params[0].name, "Value", 5) == 0);
  assert(type_decl.field_count == 1U);
  assert(strncmp(type_decl.fields[0].name, "value", 5) == 0);
  assert(strncmp(type_decl.fields[0].type_name, "Value", 5) == 0);

  beet_source_file_dispose(&file);
}

static void test_simple_choice(void) {
  const char *text = "type Option(Value) = choice {\n"
                     "    none\n"
                     "    some(Value)\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_type_decl type_decl;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_type_decl(&parser, &type_decl));
  assert(strncmp(type_decl.name, "Option", 6) == 0);
  assert(type_decl.is_structure == 0);
  assert(type_decl.is_choice == 1);
  assert(type_decl.param_count == 1U);
  assert(strncmp(type_decl.params[0].name, "Value", 5) == 0);
  assert(type_decl.variant_count == 2U);

  assert(strncmp(type_decl.variants[0].name, "none", 4) == 0);
  assert(type_decl.variants[0].has_payload == 0);

  assert(strncmp(type_decl.variants[1].name, "some", 4) == 0);
  assert(type_decl.variants[1].has_payload == 1);
  assert(strncmp(type_decl.variants[1].payload_type_name, "Value", 5) == 0);

  beet_source_file_dispose(&file);
}

static void test_choice_with_multiple_bare_variants(void) {
  const char *text = "type Bool = choice {\n"
                     "    false\n"
                     "    true\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_type_decl type_decl;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));

  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_type_decl(&parser, &type_decl));
  assert(strncmp(type_decl.name, "Bool", 4) == 0);
  assert(type_decl.is_structure == 0);
  assert(type_decl.is_choice == 1);
  assert(type_decl.param_count == 0U);
  assert(type_decl.variant_count == 2U);
  assert(type_decl.variants[0].has_payload == 0);
  assert(type_decl.variants[1].has_payload == 0);
  assert(strncmp(type_decl.variants[0].name, "false", 5) == 0);
  assert(strncmp(type_decl.variants[1].name, "true", 4) == 0);

  beet_source_file_dispose(&file);
}
int main(void) {
  test_simple_structure();
  test_parameterized_structure();
  test_simple_choice();
  test_choice_with_multiple_bare_variants();
  return 0;
}
