#include <assert.h>

#include "beet/codegen/codegen.h"
#include "beet/mir/mir.h"
#include "beet/parser/parser.h"
#include "beet/resolve/scope.h"
#include "beet/support/source.h"
#include "beet/types/check.h"
#include "beet/vm/bytecode.h"
#include "beet/vm/interpreter.h"

static void test_return_const(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 42));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 42);
}

static void test_store_and_return_local(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 10));
  assert(beet_bytecode_emit_store_local(&fn, 0, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_LOCAL, 0));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 10);
}

static void test_multiple_values(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 5));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 1, 7));
  assert(beet_bytecode_emit_store_local(&fn, 0, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_LOCAL, 0));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 7);
}

static void test_arithmetic_return_temp(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 1));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 1, 2));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 2, 3));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_MUL_INT, 3, 1, 2));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_ADD_INT, 4, 0, 3));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_TEMP, 4));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 7);
}

static void test_load_local_and_return_temp(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 10));
  assert(beet_bytecode_emit_store_local(&fn, 0, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_LOAD_LOCAL, 1, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 2, 2));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_ADD_INT, 3, 1, 2));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_TEMP, 3));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 12);
}

static void test_divide_by_zero_fails(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 8));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 1, 0));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_DIV_INT, 2, 0, 1));
  assert(!beet_vm_execute(&vm, &fn, &result));
}

static void test_comparison_return_temp(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 1));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 1, 2));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_LT_INT, 2, 0, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_TEMP, 2));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 1);
}

static void test_if_else_comparison_branch(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 3));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 1, 2));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_GT_INT, 2, 0, 1));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_JUMP_IF_FALSE, 2, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 7));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_JUMP, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 9));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 1));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 7);
}

static void test_if_else_comparison_falls_to_else(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 1));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 1, 2));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_GT_INT, 2, 0, 1));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_JUMP_IF_FALSE, 2, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 7));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_JUMP, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 9));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 1));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 9);
}

static void test_jump_skips_instructions(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_JUMP, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 99));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 7));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 7);
}

static void test_jump_if_false_takes_branch(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_JUMP_IF_FALSE, 0, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 2));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 2);
}

static void test_jump_if_false_falls_through_on_true(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 1));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_JUMP_IF_FALSE, 0, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 2));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 1);
}

static void test_jump_to_missing_label_fails(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_JUMP, 9));
  assert(!beet_vm_execute(&vm, &fn, &result));
}

static void test_jump_targets_are_precomputed(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_JUMP, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 99));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 7));

  assert(!fn.jump_targets_ready);
  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 7);
  assert(fn.jump_targets_ready);
  assert(fn.jump_target_count == 1U);
  assert(fn.jump_target_pcs[0] == 6U);
}

static void test_duplicate_labels_fail_during_jump_target_prepare(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 7));
  assert(!beet_vm_execute(&vm, &fn, &result));
}

static void test_while_false_skips_body(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_JUMP_IF_FALSE, 0, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_JUMP, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 2));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 2);
}

static void test_while_true_returns_from_body(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 1));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_JUMP_IF_FALSE, 0, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 7));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_JUMP, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 1));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_CONST_INT, 2));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 7);
}

static void test_branch_updates_local_and_returns_it(void) {
  beet_bytecode_function fn;
  beet_vm vm;
  int result;

  beet_bytecode_function_init(&fn);

  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 0, 1));
  assert(beet_bytecode_emit_store_local(&fn, 0, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 1, 1));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_JUMP_IF_FALSE, 1, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_LOAD_LOCAL, 2, 0));
  assert(beet_bytecode_emit3(&fn, BEET_BC_OP_CONST_INT, 3, 2));
  assert(beet_bytecode_emit4(&fn, BEET_BC_OP_ADD_INT, 4, 2, 3));
  assert(beet_bytecode_emit_store_local(&fn, 0, 4));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_LABEL, 0));
  assert(beet_bytecode_emit2(&fn, BEET_BC_OP_RETURN_LOCAL, 0));

  assert(beet_vm_execute(&vm, &fn, &result));
  assert(result == 3);
}

static void test_execute_lowered_expression_binding_program(void) {
  const char *text = "function main() returns Int {\n"
                     "    bind total = 1 + 2 * 3\n"
                     "    return total\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;
  beet_vm vm;
  int result;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "main.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));
  assert(beet_codegen_function(&mir_function, &bytecode_function));
  assert(beet_vm_execute(&vm, &bytecode_function, &result));
  assert(result == 7);

  beet_source_file_dispose(&file);
}

static void test_execute_lowered_structure_binding_field_access_program(void) {
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
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;
  beet_vm vm;
  int result;

  beet_source_file_init(&decl_file);
  beet_source_file_init(&function_file);
  assert(beet_source_file_set_text_copy(&decl_file, "point.beet", decl_text));
  assert(beet_source_file_set_text_copy(&function_file, "main.beet",
                                        function_text));

  beet_parser_init(&parser, &decl_file);
  assert(beet_parser_parse_type_decl(&parser, &type_decl));

  beet_parser_init(&parser, &function_file);
  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_type_decl(&type_decl));
  assert(beet_type_check_function_signature_with_type_decls(&function_ast,
                                                            &type_decl, 1U));
  assert(beet_type_check_function_body_with_type_decls(&function_ast,
                                                       &type_decl, 1U));
  assert(beet_mir_lower_function(&mir_function, &function_ast));
  assert(beet_codegen_function(&mir_function, &bytecode_function));
  assert(beet_vm_execute(&vm, &bytecode_function, &result));
  assert(result == 3);

  beet_source_file_dispose(&function_file);
  beet_source_file_dispose(&decl_file);
}

static void test_execute_program_function_call(void) {
  beet_bytecode_program program;
  beet_vm vm;
  int args[1];
  int result;

  beet_bytecode_program_init(&program);
  program.function_count = 2U;
  beet_bytecode_function_init(&program.functions[0]);
  beet_bytecode_function_init(&program.functions[1]);

  program.functions[0].param_count = 1U;
  assert(
      beet_bytecode_emit3(&program.functions[0], BEET_BC_OP_LOAD_LOCAL, 0, 0));
  assert(
      beet_bytecode_emit3(&program.functions[0], BEET_BC_OP_CONST_INT, 1, 1));
  assert(
      beet_bytecode_emit4(&program.functions[0], BEET_BC_OP_ADD_INT, 2, 0, 1));
  assert(beet_bytecode_emit2(&program.functions[0], BEET_BC_OP_RETURN_TEMP, 2));

  args[0] = 0;
  assert(
      beet_bytecode_emit3(&program.functions[1], BEET_BC_OP_CONST_INT, 0, 41));
  assert(beet_bytecode_emit_call(&program.functions[1], 1, 0, args, 1U));
  assert(beet_bytecode_emit2(&program.functions[1], BEET_BC_OP_RETURN_TEMP, 1));

  assert(beet_vm_execute_program(&vm, &program, 1U, &result));
  assert(result == 42);
}

static void test_execute_program_recursive_call(void) {
  beet_bytecode_program program;
  beet_vm vm;
  int args[1];
  int result;

  beet_bytecode_program_init(&program);
  program.function_count = 2U;
  beet_bytecode_function_init(&program.functions[0]);
  beet_bytecode_function_init(&program.functions[1]);

  program.functions[0].param_count = 1U;
  assert(
      beet_bytecode_emit3(&program.functions[0], BEET_BC_OP_LOAD_LOCAL, 0, 0));
  assert(
      beet_bytecode_emit3(&program.functions[0], BEET_BC_OP_CONST_INT, 1, 0));
  assert(
      beet_bytecode_emit4(&program.functions[0], BEET_BC_OP_EQ_INT, 2, 0, 1));
  assert(beet_bytecode_emit3(&program.functions[0], BEET_BC_OP_JUMP_IF_FALSE, 2,
                             0));
  assert(beet_bytecode_emit2(&program.functions[0], BEET_BC_OP_RETURN_CONST_INT,
                             0));
  assert(beet_bytecode_emit2(&program.functions[0], BEET_BC_OP_LABEL, 0));
  assert(
      beet_bytecode_emit3(&program.functions[0], BEET_BC_OP_LOAD_LOCAL, 3, 0));
  assert(
      beet_bytecode_emit3(&program.functions[0], BEET_BC_OP_CONST_INT, 4, 1));
  assert(
      beet_bytecode_emit4(&program.functions[0], BEET_BC_OP_SUB_INT, 5, 3, 4));
  args[0] = 5;
  assert(beet_bytecode_emit_call(&program.functions[0], 6, 0, args, 1U));
  assert(
      beet_bytecode_emit3(&program.functions[0], BEET_BC_OP_CONST_INT, 7, 1));
  assert(
      beet_bytecode_emit4(&program.functions[0], BEET_BC_OP_ADD_INT, 8, 6, 7));
  assert(beet_bytecode_emit2(&program.functions[0], BEET_BC_OP_RETURN_TEMP, 8));

  args[0] = 0;
  assert(
      beet_bytecode_emit3(&program.functions[1], BEET_BC_OP_CONST_INT, 0, 3));
  assert(beet_bytecode_emit_call(&program.functions[1], 1, 0, args, 1U));
  assert(beet_bytecode_emit2(&program.functions[1], BEET_BC_OP_RETURN_TEMP, 1));

  assert(beet_vm_execute_program(&vm, &program, 1U, &result));
  assert(result == 3);
}

static void test_execute_match_on_local_choice_binding(void) {
  const char *decl_text = "type MaybeInt = choice {\n"
                          "    none\n"
                          "    some(Int)\n"
                          "}\n";
  const char *function_text = "function main() returns Int {\n"
                              "    bind value is MaybeInt = MaybeInt.some(7)\n"
                              "    match value {\n"
                              "        case none {\n"
                              "            return 0\n"
                              "        }\n"
                              "        case some(item) {\n"
                              "            return item\n"
                              "        }\n"
                              "    }\n"
                              "    return 1\n"
                              "}\n";
  beet_source_file decl_file;
  beet_source_file function_file;
  beet_parser parser;
  beet_ast_type_decl type_decl;
  beet_ast_function function_ast;
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;
  beet_vm vm;
  int result;

  beet_source_file_init(&decl_file);
  beet_source_file_init(&function_file);
  assert(beet_source_file_set_text_copy(&decl_file, "option.beet", decl_text));
  assert(beet_source_file_set_text_copy(&function_file, "main.beet",
                                        function_text));

  beet_parser_init(&parser, &decl_file);
  assert(beet_parser_parse_type_decl(&parser, &type_decl));

  beet_parser_init(&parser, &function_file);
  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_type_decl(&type_decl));
  assert(beet_type_check_function_signature_with_type_decls(&function_ast,
                                                            &type_decl, 1U));
  assert(beet_type_check_function_body_with_type_decls(&function_ast,
                                                       &type_decl, 1U));
  assert(beet_mir_lower_function_with_type_decls(&mir_function, &function_ast,
                                                 &type_decl, 1U));
  assert(beet_codegen_function(&mir_function, &bytecode_function));
  assert(beet_vm_execute(&vm, &bytecode_function, &result));
  assert(result == 7);

  beet_source_file_dispose(&function_file);
  beet_source_file_dispose(&decl_file);
}

static void test_execute_match_on_empty_choice_variant(void) {
  const char *decl_text = "type MaybeInt = choice {\n"
                          "    none\n"
                          "    some(Int)\n"
                          "}\n";
  const char *function_text = "function main() returns Int {\n"
                              "    bind value is MaybeInt = MaybeInt.none()\n"
                              "    match value {\n"
                              "        case none {\n"
                              "            return 0\n"
                              "        }\n"
                              "        case some(item) {\n"
                              "            return item\n"
                              "        }\n"
                              "    }\n"
                              "    return 1\n"
                              "}\n";
  beet_source_file decl_file;
  beet_source_file function_file;
  beet_parser parser;
  beet_ast_type_decl type_decl;
  beet_ast_function function_ast;
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;
  beet_vm vm;
  int result;

  beet_source_file_init(&decl_file);
  beet_source_file_init(&function_file);
  assert(beet_source_file_set_text_copy(&decl_file, "option.beet", decl_text));
  assert(beet_source_file_set_text_copy(&function_file, "main.beet",
                                        function_text));

  beet_parser_init(&parser, &decl_file);
  assert(beet_parser_parse_type_decl(&parser, &type_decl));

  beet_parser_init(&parser, &function_file);
  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_type_decl(&type_decl));
  assert(beet_type_check_function_signature_with_type_decls(&function_ast,
                                                            &type_decl, 1U));
  assert(beet_type_check_function_body_with_type_decls(&function_ast,
                                                       &type_decl, 1U));
  assert(beet_mir_lower_function_with_type_decls(&mir_function, &function_ast,
                                                 &type_decl, 1U));
  assert(beet_codegen_function(&mir_function, &bytecode_function));
  assert(beet_vm_execute(&vm, &bytecode_function, &result));
  assert(result == 0);

  beet_source_file_dispose(&function_file);
  beet_source_file_dispose(&decl_file);
}

int main(void) {
  test_return_const();
  test_store_and_return_local();
  test_multiple_values();
  test_arithmetic_return_temp();
  test_load_local_and_return_temp();
  test_divide_by_zero_fails();
  test_comparison_return_temp();
  test_if_else_comparison_branch();
  test_if_else_comparison_falls_to_else();
  test_jump_skips_instructions();
  test_jump_if_false_takes_branch();
  test_jump_if_false_falls_through_on_true();
  test_jump_to_missing_label_fails();
  test_jump_targets_are_precomputed();
  test_duplicate_labels_fail_during_jump_target_prepare();
  test_while_false_skips_body();
  test_while_true_returns_from_body();
  test_branch_updates_local_and_returns_it();
  test_execute_lowered_expression_binding_program();
  test_execute_lowered_structure_binding_field_access_program();
  test_execute_program_function_call();
  test_execute_program_recursive_call();
  test_execute_match_on_local_choice_binding();
  test_execute_match_on_empty_choice_variant();
  return 0;
}
