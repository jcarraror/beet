#include <assert.h>
#include <string.h>

#include "beet/parser/parser.h"
#include "beet/support/source.h"
#include "beet/types/check.h"
#include "beet/types/types.h"

static void test_infer_int_binding(void) {
  const char *text = "bind x = 10\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;
  beet_type type;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));
  type = beet_type_check_binding(&binding);

  assert(type.kind == BEET_TYPE_INT);
  assert(strcmp(type.name, "Int") == 0);

  beet_source_file_dispose(&file);
}

static void test_typed_binding_ok(void) {
  const char *text = "bind x is Int = 5\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;
  beet_type_check_result result;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));
  result = beet_type_check_binding_annotation(&binding);

  assert(result.ok);
  assert(result.declared_type.kind == BEET_TYPE_INT);
  assert(result.value_type.kind == BEET_TYPE_INT);

  beet_source_file_dispose(&file);
}

static void test_typed_binding_mismatch(void) {
  const char *text = "bind x is Bool = 5\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;
  beet_type_check_result result;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));
  result = beet_type_check_binding_annotation(&binding);

  assert(!result.ok);
  assert(result.declared_type.kind == BEET_TYPE_BOOL);
  assert(result.value_type.kind == BEET_TYPE_INT);

  beet_source_file_dispose(&file);
}

static void test_function_signature_types(void) {
  const char *text = "function add(x is Int, y is Int) returns Int {\n"
                     "    return 0\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_type_check_function_signature(&function_ast));

  beet_source_file_dispose(&file);
}

static void test_type_decl_fields_known(void) {
  const char *text = "type Point = structure {\n"
                     "    x is Int\n"
                     "    y is Float\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_type_decl type_decl;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_type_decl(&parser, &type_decl));
  assert(beet_type_check_type_decl(&type_decl));

  beet_source_file_dispose(&file);
}

static void test_function_body_arithmetic_ok(void) {
  const char *text = "function add(x is Int, y is Int) returns Int {\n"
                     "    return x + y * 2\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));

  beet_source_file_dispose(&file);
}

static void test_function_body_unary_grouped_int_ok(void) {
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
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));

  beet_source_file_dispose(&file);
}

static void test_function_body_arithmetic_rejects_non_int(void) {
  const char *text = "function bad(flag is Bool) returns Int {\n"
                     "    return flag + 1\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_type_check_function_signature(&function_ast));
  assert(!beet_type_check_function_body(&function_ast));

  beet_source_file_dispose(&file);
}

static void test_if_condition_bool_ok(void) {
  const char *text = "function choose(flag is Bool) returns Int {\n"
                     "    if flag {\n"
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
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));

  beet_source_file_dispose(&file);
}

static void test_if_condition_rejects_non_bool(void) {
  const char *text = "function choose(x is Int) returns Int {\n"
                     "    if x {\n"
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
  assert(beet_type_check_function_signature(&function_ast));
  assert(!beet_type_check_function_body(&function_ast));

  beet_source_file_dispose(&file);
}

static void test_if_body_binding_does_not_escape_scope(void) {
  const char *text = "function choose(flag is Bool) returns Int {\n"
                     "    if flag {\n"
                     "        bind x = 1\n"
                     "        return x\n"
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
  assert(beet_type_check_function_signature(&function_ast));
  assert(!beet_type_check_function_body(&function_ast));

  beet_source_file_dispose(&file);
}

int main(void) {
  test_infer_int_binding();
  test_typed_binding_ok();
  test_typed_binding_mismatch();
  test_function_signature_types();
  test_type_decl_fields_known();
  test_function_body_arithmetic_ok();
  test_function_body_unary_grouped_int_ok();
  test_function_body_arithmetic_rejects_non_int();
  test_if_condition_bool_ok();
  test_if_condition_rejects_non_bool();
  test_if_body_binding_does_not_escape_scope();
  return 0;
}
