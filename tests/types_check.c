#include <assert.h>
#include <string.h>

#include "beet/parser/parser.h"
#include "beet/resolve/scope.h"
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

static void test_infer_bool_binding(void) {
  const char *text = "bind flag = true\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;
  beet_type type;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));
  type = beet_type_check_binding(&binding);

  assert(type.kind == BEET_TYPE_BOOL);
  assert(strcmp(type.name, "Bool") == 0);

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

static void test_typed_binding_expression_ok(void) {
  const char *text = "bind x is Int = 1 + 2\n";
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

static void test_parameterized_structure_type_decl_ok(void) {
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
  assert(beet_type_check_type_decl(&type_decl));

  beet_source_file_dispose(&file);
}

static void test_parameterized_choice_type_decl_ok(void) {
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
  assert(beet_type_check_type_decl(&type_decl));

  beet_source_file_dispose(&file);
}

static void test_type_decls_resolve_named_types_across_decls(void) {
  const char *text = "type Point = structure {\n"
                     "    x is Int\n"
                     "    y is Int\n"
                     "}\n"
                     "type Line = structure {\n"
                     "    start is Point\n"
                     "    end is Point\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_type_decl decls[2];

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_type_decl(&parser, &decls[0]));
  assert(beet_parser_parse_type_decl(&parser, &decls[1]));
  assert(beet_type_check_type_decls(decls, 2U));

  beet_source_file_dispose(&file);
}

static void test_type_decl_rejects_unknown_named_type(void) {
  const char *text = "type Box = structure {\n"
                     "    value is Missing\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_type_decl type_decl;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_type_decl(&parser, &type_decl));
  assert(beet_type_check_type_decl(&type_decl) == 0);

  beet_source_file_dispose(&file);
}

static void test_type_decls_reject_duplicate_names(void) {
  const char *text = "type Point = structure {\n"
                     "    x is Int\n"
                     "}\n"
                     "type Point = choice {\n"
                     "    none\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_type_decl decls[2];

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_type_decl(&parser, &decls[0]));
  assert(beet_parser_parse_type_decl(&parser, &decls[1]));
  assert(beet_type_check_type_decls(decls, 2U) == 0);

  beet_source_file_dispose(&file);
}

static void test_choice_type_decl_rejects_duplicate_variants(void) {
  const char *text = "type Bool = choice {\n"
                     "    true\n"
                     "    true\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_type_decl type_decl;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_type_decl(&parser, &type_decl));
  assert(beet_type_check_type_decl(&type_decl) == 0);

  beet_source_file_dispose(&file);
}

static void test_function_signature_rejects_unknown_type_name(void) {
  const char *text = "function main(x is Missing) returns Int {\n"
                     "    return 0\n"
                     "}\n";

  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_type_check_function_signature(&function_ast) == 0);

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

static void test_function_body_binding_expression_uses_local_ok(void) {
  const char *text = "function add_one(x is Int) returns Int {\n"
                     "    bind y = x + 1\n"
                     "    return y\n"
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

static void test_function_body_bool_return_ok(void) {
  const char *text = "function main() returns Bool {\n"
                     "    return true\n"
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

static void test_function_body_comparison_ok(void) {
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
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));

  beet_source_file_dispose(&file);
}

static void test_function_body_comparison_rejects_non_int(void) {
  const char *text = "function main(flag is Bool) returns Bool {\n"
                     "    return flag == true\n"
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

static void test_if_else_branches_ok(void) {
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
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));

  beet_source_file_dispose(&file);
}

static void test_else_body_binding_does_not_escape_scope(void) {
  const char *text = "function choose(flag is Bool) returns Int {\n"
                     "    if flag {\n"
                     "        return 1\n"
                     "    } else {\n"
                     "        bind x = 2\n"
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

static void test_mutable_assignment_ok(void) {
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
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));

  beet_source_file_dispose(&file);
}

static void test_assignment_rejects_immutable_target(void) {
  const char *text = "function main() returns Int {\n"
                     "    bind total = 1\n"
                     "    total = 2\n"
                     "    return total\n"
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

static void test_assignment_rejects_type_mismatch(void) {
  const char *text = "function main() returns Int {\n"
                     "    mutable total = 1\n"
                     "    total = true\n"
                     "    return total\n"
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

static void test_function_body_field_access_on_named_param_ok(void) {
  const char *decl_text = "type Point = structure {\n"
                          "    x is Int\n"
                          "    y is Int\n"
                          "}\n";
  const char *function_text = "function get(point is Point) returns Int {\n"
                              "    return point.x\n"
                              "}\n";
  beet_source_file decl_file;
  beet_source_file function_file;
  beet_parser parser;
  beet_ast_type_decl type_decl;
  beet_ast_function function_ast;

  beet_source_file_init(&decl_file);
  beet_source_file_init(&function_file);
  assert(beet_source_file_set_text_copy(&decl_file, "point.beet", decl_text));
  assert(beet_source_file_set_text_copy(&function_file, "get.beet",
                                        function_text));

  beet_parser_init(&parser, &decl_file);
  assert(beet_parser_parse_type_decl(&parser, &type_decl));

  beet_parser_init(&parser, &function_file);
  assert(beet_parser_parse_function(&parser, &function_ast));

  assert(beet_type_check_type_decl(&type_decl));
  assert(beet_type_check_function_signature_with_type_decls(&function_ast,
                                                            &type_decl, 1U));
  assert(beet_type_check_function_body_with_type_decls(&function_ast,
                                                       &type_decl, 1U));

  beet_source_file_dispose(&function_file);
  beet_source_file_dispose(&decl_file);
}

static void
test_function_body_structure_construction_and_field_access_ok(void) {
  const char *decl_text = "type Point = structure {\n"
                          "    x is Int\n"
                          "    y is Int\n"
                          "}\n";
  const char *function_text = "function main() returns Int {\n"
                              "    return Point(x = 3, y = 4).x\n"
                              "}\n";
  beet_source_file decl_file;
  beet_source_file function_file;
  beet_parser parser;
  beet_ast_type_decl type_decl;
  beet_ast_function function_ast;

  beet_source_file_init(&decl_file);
  beet_source_file_init(&function_file);
  assert(beet_source_file_set_text_copy(&decl_file, "point.beet", decl_text));
  assert(beet_source_file_set_text_copy(&function_file, "main.beet",
                                        function_text));

  beet_parser_init(&parser, &decl_file);
  assert(beet_parser_parse_type_decl(&parser, &type_decl));

  beet_parser_init(&parser, &function_file);
  assert(beet_parser_parse_function(&parser, &function_ast));

  assert(beet_type_check_type_decl(&type_decl));
  assert(beet_type_check_function_signature_with_type_decls(&function_ast,
                                                            &type_decl, 1U));
  assert(beet_type_check_function_body_with_type_decls(&function_ast,
                                                       &type_decl, 1U));

  beet_source_file_dispose(&function_file);
  beet_source_file_dispose(&decl_file);
}

static void
test_function_body_structure_construction_rejects_field_type_mismatch(void) {
  const char *decl_text = "type Point = structure {\n"
                          "    x is Int\n"
                          "    y is Int\n"
                          "}\n";
  const char *function_text = "function main() returns Int {\n"
                              "    return Point(x = true, y = 4).x\n"
                              "}\n";
  beet_source_file decl_file;
  beet_source_file function_file;
  beet_parser parser;
  beet_ast_type_decl type_decl;
  beet_ast_function function_ast;

  beet_source_file_init(&decl_file);
  beet_source_file_init(&function_file);
  assert(beet_source_file_set_text_copy(&decl_file, "point.beet", decl_text));
  assert(beet_source_file_set_text_copy(&function_file, "main.beet",
                                        function_text));

  beet_parser_init(&parser, &decl_file);
  assert(beet_parser_parse_type_decl(&parser, &type_decl));

  beet_parser_init(&parser, &function_file);
  assert(beet_parser_parse_function(&parser, &function_ast));

  assert(beet_type_check_type_decl(&type_decl));
  assert(beet_type_check_function_signature_with_type_decls(&function_ast,
                                                            &type_decl, 1U));
  assert(!beet_type_check_function_body_with_type_decls(&function_ast,
                                                        &type_decl, 1U));

  beet_source_file_dispose(&function_file);
  beet_source_file_dispose(&decl_file);
}

static void test_function_body_field_access_rejects_unknown_field(void) {
  const char *decl_text = "type Point = structure {\n"
                          "    x is Int\n"
                          "    y is Int\n"
                          "}\n";
  const char *function_text = "function get(point is Point) returns Int {\n"
                              "    return point.z\n"
                              "}\n";
  beet_source_file decl_file;
  beet_source_file function_file;
  beet_parser parser;
  beet_ast_type_decl type_decl;
  beet_ast_function function_ast;

  beet_source_file_init(&decl_file);
  beet_source_file_init(&function_file);
  assert(beet_source_file_set_text_copy(&decl_file, "point.beet", decl_text));
  assert(beet_source_file_set_text_copy(&function_file, "get.beet",
                                        function_text));

  beet_parser_init(&parser, &decl_file);
  assert(beet_parser_parse_type_decl(&parser, &type_decl));

  beet_parser_init(&parser, &function_file);
  assert(beet_parser_parse_function(&parser, &function_ast));

  assert(beet_type_check_type_decl(&type_decl));
  assert(beet_type_check_function_signature_with_type_decls(&function_ast,
                                                            &type_decl, 1U));
  assert(!beet_type_check_function_body_with_type_decls(&function_ast,
                                                        &type_decl, 1U));

  beet_source_file_dispose(&function_file);
  beet_source_file_dispose(&decl_file);
}

static void test_function_body_structure_binding_and_field_access_ok(void) {
  const char *decl_text = "type Point = structure {\n"
                          "    x is Int\n"
                          "    y is Int\n"
                          "}\n";
  const char *function_text = "function main() returns Int {\n"
                              "    bind point is Point = Point(x = 3, y = 4)\n"
                              "    return point.x\n"
                              "}\n";
  beet_source_file decl_file;
  beet_source_file function_file;
  beet_parser parser;
  beet_ast_type_decl type_decl;
  beet_ast_function function_ast;

  beet_source_file_init(&decl_file);
  beet_source_file_init(&function_file);
  assert(beet_source_file_set_text_copy(&decl_file, "point.beet", decl_text));
  assert(beet_source_file_set_text_copy(&function_file, "main.beet",
                                        function_text));

  beet_parser_init(&parser, &decl_file);
  assert(beet_parser_parse_type_decl(&parser, &type_decl));

  beet_parser_init(&parser, &function_file);
  assert(beet_parser_parse_function(&parser, &function_ast));

  assert(beet_type_check_type_decl(&type_decl));
  assert(beet_type_check_function_signature_with_type_decls(&function_ast,
                                                            &type_decl, 1U));
  assert(beet_type_check_function_body_with_type_decls(&function_ast,
                                                       &type_decl, 1U));

  beet_source_file_dispose(&function_file);
  beet_source_file_dispose(&decl_file);
}

static void test_function_call_body_ok(void) {
  const char *text = "function add(x is Int, y is Int) returns Int {\n"
                     "    return x + y\n"
                     "}\n"
                     "function main() returns Int {\n"
                     "    return add(1, 2)\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function functions[2];

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &functions[0]));
  assert(beet_parser_parse_function(&parser, &functions[1]));
  assert(beet_type_check_function_signature(&functions[0]));
  assert(beet_type_check_function_signature(&functions[1]));
  assert(beet_resolve_function_with_decls(&functions[1], functions, 2U));
  assert(beet_type_check_function_body_with_decls(&functions[1], NULL, 0U,
                                                  functions, 2U));

  beet_source_file_dispose(&file);
}

static void test_function_call_rejects_argument_type_mismatch(void) {
  const char *text = "function add(x is Int, y is Int) returns Int {\n"
                     "    return x + y\n"
                     "}\n"
                     "function main() returns Int {\n"
                     "    return add(true, 2)\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function functions[2];

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &functions[0]));
  assert(beet_parser_parse_function(&parser, &functions[1]));
  assert(beet_type_check_function_signature(&functions[0]));
  assert(beet_type_check_function_signature(&functions[1]));
  assert(beet_resolve_function_with_decls(&functions[1], functions, 2U));
  assert(!beet_type_check_function_body_with_decls(&functions[1], NULL, 0U,
                                                   functions, 2U));

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

static void test_while_condition_bool_ok(void) {
  const char *text = "function main() returns Int {\n"
                     "    while true {\n"
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
  test_infer_bool_binding();
  test_typed_binding_ok();
  test_typed_binding_mismatch();
  test_typed_binding_expression_ok();
  test_function_signature_types();
  test_type_decl_fields_known();
  test_parameterized_structure_type_decl_ok();
  test_parameterized_choice_type_decl_ok();
  test_type_decls_resolve_named_types_across_decls();
  test_type_decl_rejects_unknown_named_type();
  test_type_decls_reject_duplicate_names();
  test_choice_type_decl_rejects_duplicate_variants();
  test_function_signature_rejects_unknown_type_name();
  test_function_body_arithmetic_ok();
  test_function_body_binding_expression_uses_local_ok();
  test_function_body_unary_grouped_int_ok();
  test_function_body_arithmetic_rejects_non_int();
  test_function_body_bool_return_ok();
  test_function_body_comparison_ok();
  test_function_body_comparison_rejects_non_int();
  test_if_else_branches_ok();
  test_else_body_binding_does_not_escape_scope();
  test_mutable_assignment_ok();
  test_assignment_rejects_immutable_target();
  test_assignment_rejects_type_mismatch();
  test_function_body_field_access_on_named_param_ok();
  test_function_body_structure_construction_and_field_access_ok();
  test_function_body_structure_construction_rejects_field_type_mismatch();
  test_function_body_field_access_rejects_unknown_field();
  test_function_body_structure_binding_and_field_access_ok();
  test_function_call_body_ok();
  test_function_call_rejects_argument_type_mismatch();
  test_if_condition_bool_ok();
  test_if_condition_rejects_non_bool();
  test_while_condition_bool_ok();
  test_if_body_binding_does_not_escape_scope();
  return 0;
}
