#include <assert.h>
#include <string.h>

#include "beet/mir/mir.h"
#include "beet/parser/parser.h"
#include "beet/resolve/scope.h"
#include "beet/semantics/registry.h"
#include "beet/support/source.h"
#include "beet/types/check.h"

static void test_registry_shared_across_frontend_layers(void) {
  const char *text = "type MaybeInt = choice {\n"
                     "    none\n"
                     "    some(Int)\n"
                     "}\n"
                     "\n"
                     "function helper(x is Int) returns Int {\n"
                     "    return x\n"
                     "}\n"
                     "\n"
                     "function main() returns Int {\n"
                     "    bind value is MaybeInt = MaybeInt.some(7)\n"
                     "    match value {\n"
                     "        case none {\n"
                     "            return 0\n"
                     "        }\n"
                     "        case some(item) {\n"
                     "            return helper(item)\n"
                     "        }\n"
                     "    }\n"
                     "    return 1\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_decl_registry registry;
  const beet_ast_type_decl *lookup_type;
  const beet_ast_function *lookup_function;
  const beet_ast_choice_variant *lookup_variant;
  size_t function_index;
  size_t variant_index;
  static beet_ast_type_decl type_decl;
  static beet_ast_function functions[2];
  static beet_mir_function mir_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_type_decl(&parser, &type_decl));
  assert(beet_parser_parse_function(&parser, &functions[0]));
  assert(beet_parser_parse_function(&parser, &functions[1]));

  beet_decl_registry_init(&registry, &type_decl, 1U, functions, 2U);

  lookup_type = beet_decl_registry_find_type(&registry, "MaybeInt", 8U);
  assert(lookup_type == &type_decl);

  lookup_function = beet_decl_registry_find_function(&registry, "helper", 6U);
  assert(lookup_function == &functions[0]);
  assert(beet_decl_registry_find_function_index(&registry, "main", 4U,
                                                &function_index));
  assert(function_index == 1U);

  lookup_variant =
      beet_decl_registry_find_choice_variant(&type_decl, "some", 4U);
  assert(lookup_variant != NULL);
  assert(lookup_variant->has_payload == 1);
  assert(beet_decl_registry_find_choice_variant_index(&type_decl, "none", 4U,
                                                      &variant_index));
  assert(variant_index == 0U);

  assert(beet_type_check_type_decls_with_registry(&registry));
  assert(beet_type_check_function_signature_with_registry(&functions[0],
                                                          &registry));
  assert(beet_type_check_function_signature_with_registry(&functions[1],
                                                          &registry));
  assert(beet_resolve_function_with_registry(&functions[0], &registry));
  assert(beet_resolve_function_with_registry(&functions[1], &registry));
  assert(beet_type_check_function_body_with_registry(&functions[0], &registry));
  assert(beet_type_check_function_body_with_registry(&functions[1], &registry));
  assert(beet_mir_lower_function_with_registry(&mir_function, &functions[1],
                                               &registry));

  assert(functions[1].body[1].match_expr.kind == BEET_AST_EXPR_NAME);
  assert(functions[1].body[1].match_expr.is_resolved == 1);
  assert(functions[1].body[1].match_case_count == 2U);
  assert(functions[1].body[1].match_cases[1].body[0].expr.call_is_resolved ==
         1);
  assert(strcmp(mir_function.locals[0], "value.tag") == 0);

  beet_source_file_dispose(&file);
}

int main(void) {
  test_registry_shared_across_frontend_layers();
  return 0;
}
