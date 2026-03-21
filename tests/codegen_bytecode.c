#include <assert.h>
#include <string.h>

#include "beet/codegen/codegen.h"
#include "beet/mir/mir.h"
#include "beet/parser/parser.h"
#include "beet/resolve/scope.h"
#include "beet/support/source.h"
#include "beet/types/check.h"
#include "beet/vm/bytecode.h"

static void test_codegen_binding(void) {
  const char *text = "bind x = 10\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));

  beet_mir_function_init(&mir_function, "main", 4U);
  assert(beet_mir_lower_binding(&mir_function, &binding));
  assert(beet_codegen_function(&mir_function, &bytecode_function));

  assert(bytecode_function.code_count == 6U);

  assert(bytecode_function.code[0] == BEET_BC_OP_CONST_INT);
  assert(bytecode_function.code[1] == 0);
  assert(bytecode_function.code[2] == 10);

  assert(bytecode_function.code[3] == BEET_BC_OP_STORE_LOCAL);
  assert(bytecode_function.code[4] == 0);
  assert(bytecode_function.code[5] == 0);

  beet_source_file_dispose(&file);
}

static void test_codegen_return_const(void) {
  const char *text = "function main() returns Int {\n"
                     "    return 0\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(
      beet_mir_lower_trivial_return_function(&mir_function, &function_ast, 0));
  assert(beet_codegen_function(&mir_function, &bytecode_function));

  assert(bytecode_function.code_count == 2U);
  assert(bytecode_function.code[0] == BEET_BC_OP_RETURN_CONST_INT);
  assert(bytecode_function.code[1] == 0);

  beet_source_file_dispose(&file);
}

static void test_codegen_return_local(void) {
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;

  beet_mir_function_init(&mir_function, "main", 4U);

  assert(beet_mir_add_const_int(&mir_function, 7) == 0);
  assert(beet_mir_add_bind_local(&mir_function, "x", 1U, 0));
  assert(beet_mir_add_return_local(&mir_function, "x", 1U));

  assert(beet_codegen_function(&mir_function, &bytecode_function));

  assert(bytecode_function.code_count == 8U);

  assert(bytecode_function.code[0] == BEET_BC_OP_CONST_INT);
  assert(bytecode_function.code[1] == 0);
  assert(bytecode_function.code[2] == 7);

  assert(bytecode_function.code[3] == BEET_BC_OP_STORE_LOCAL);
  assert(bytecode_function.code[4] == 0);
  assert(bytecode_function.code[5] == 0);

  assert(bytecode_function.code[6] == BEET_BC_OP_RETURN_LOCAL);
  assert(bytecode_function.code[7] == 0);
}

static void test_codegen_lowered_function_return_local(void) {
  const char *text = "function main() returns Int {\n"
                     "    bind x = 10\n"
                     "    return x\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));
  assert(beet_codegen_function(&mir_function, &bytecode_function));

  assert(bytecode_function.code_count == 8U);
  assert(bytecode_function.code[0] == BEET_BC_OP_CONST_INT);
  assert(bytecode_function.code[1] == 0);
  assert(bytecode_function.code[2] == 10);
  assert(bytecode_function.code[3] == BEET_BC_OP_STORE_LOCAL);
  assert(bytecode_function.code[4] == 0);
  assert(bytecode_function.code[5] == 0);
  assert(bytecode_function.code[6] == BEET_BC_OP_RETURN_LOCAL);
  assert(bytecode_function.code[7] == 0);

  beet_source_file_dispose(&file);
}

static void test_codegen_lowered_arithmetic_return_expression(void) {
  const char *text = "function main() returns Int {\n"
                     "    return 1 + 2 * 3\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));
  assert(beet_codegen_function(&mir_function, &bytecode_function));

  assert(bytecode_function.code_count == 19U);
  assert(bytecode_function.code[0] == BEET_BC_OP_CONST_INT);
  assert(bytecode_function.code[1] == 0);
  assert(bytecode_function.code[2] == 1);
  assert(bytecode_function.code[3] == BEET_BC_OP_CONST_INT);
  assert(bytecode_function.code[4] == 1);
  assert(bytecode_function.code[5] == 2);
  assert(bytecode_function.code[6] == BEET_BC_OP_CONST_INT);
  assert(bytecode_function.code[7] == 2);
  assert(bytecode_function.code[8] == 3);
  assert(bytecode_function.code[9] == BEET_BC_OP_MUL_INT);
  assert(bytecode_function.code[10] == 3);
  assert(bytecode_function.code[11] == 1);
  assert(bytecode_function.code[12] == 2);
  assert(bytecode_function.code[13] == BEET_BC_OP_ADD_INT);
  assert(bytecode_function.code[14] == 4);
  assert(bytecode_function.code[15] == 0);
  assert(bytecode_function.code[16] == 3);
  assert(bytecode_function.code[17] == BEET_BC_OP_RETURN_TEMP);
  assert(bytecode_function.code[18] == 4);

  beet_source_file_dispose(&file);
}

static void test_codegen_lowered_arithmetic_with_local_load(void) {
  const char *text = "function main() returns Int {\n"
                     "    bind x = 10\n"
                     "    return x + 2\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));
  assert(beet_codegen_function(&mir_function, &bytecode_function));

  assert(bytecode_function.code_count == 18U);
  assert(bytecode_function.code[0] == BEET_BC_OP_CONST_INT);
  assert(bytecode_function.code[1] == 0);
  assert(bytecode_function.code[2] == 10);
  assert(bytecode_function.code[3] == BEET_BC_OP_STORE_LOCAL);
  assert(bytecode_function.code[4] == 0);
  assert(bytecode_function.code[5] == 0);
  assert(bytecode_function.code[6] == BEET_BC_OP_LOAD_LOCAL);
  assert(bytecode_function.code[7] == 1);
  assert(bytecode_function.code[8] == 0);
  assert(bytecode_function.code[9] == BEET_BC_OP_CONST_INT);
  assert(bytecode_function.code[10] == 2);
  assert(bytecode_function.code[11] == 2);
  assert(bytecode_function.code[12] == BEET_BC_OP_ADD_INT);
  assert(bytecode_function.code[13] == 3);
  assert(bytecode_function.code[14] == 1);
  assert(bytecode_function.code[15] == 2);
  assert(bytecode_function.code[16] == BEET_BC_OP_RETURN_TEMP);
  assert(bytecode_function.code[17] == 3);

  beet_source_file_dispose(&file);
}

static void test_codegen_lowered_if_statement(void) {
  const char *text = "function choose(flag is Bool) returns Int {\n"
                     "    if flag {\n"
                     "        return 1\n"
                     "    }\n"
                     "    return 0\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));
  assert(beet_codegen_function(&mir_function, &bytecode_function));

  assert(bytecode_function.local_count == 1U);
  assert(bytecode_function.code_count == 12U);
  assert(bytecode_function.code[0] == BEET_BC_OP_LOAD_LOCAL);
  assert(bytecode_function.code[1] == 0);
  assert(bytecode_function.code[2] == 0);
  assert(bytecode_function.code[3] == BEET_BC_OP_JUMP_IF_FALSE);
  assert(bytecode_function.code[4] == 0);
  assert(bytecode_function.code[5] == 0);
  assert(bytecode_function.code[6] == BEET_BC_OP_RETURN_CONST_INT);
  assert(bytecode_function.code[7] == 1);
  assert(bytecode_function.code[8] == BEET_BC_OP_LABEL);
  assert(bytecode_function.code[9] == 0);
  assert(bytecode_function.code[10] == BEET_BC_OP_RETURN_CONST_INT);
  assert(bytecode_function.code[11] == 0);

  beet_source_file_dispose(&file);
}

static void test_codegen_lowered_while_statement(void) {
  const char *text = "function main() returns Int {\n"
                     "    while true {\n"
                     "        return 1\n"
                     "    }\n"
                     "    return 0\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));
  assert(beet_codegen_function(&mir_function, &bytecode_function));

  assert(bytecode_function.code_count == 16U);
  assert(bytecode_function.code[0] == BEET_BC_OP_LABEL);
  assert(bytecode_function.code[1] == 0);
  assert(bytecode_function.code[2] == BEET_BC_OP_CONST_INT);
  assert(bytecode_function.code[3] == 0);
  assert(bytecode_function.code[4] == 1);
  assert(bytecode_function.code[5] == BEET_BC_OP_JUMP_IF_FALSE);
  assert(bytecode_function.code[6] == 0);
  assert(bytecode_function.code[7] == 1);
  assert(bytecode_function.code[8] == BEET_BC_OP_RETURN_CONST_INT);
  assert(bytecode_function.code[9] == 1);
  assert(bytecode_function.code[10] == BEET_BC_OP_JUMP);
  assert(bytecode_function.code[11] == 0);
  assert(bytecode_function.code[12] == BEET_BC_OP_LABEL);
  assert(bytecode_function.code[13] == 1);
  assert(bytecode_function.code[14] == BEET_BC_OP_RETURN_CONST_INT);
  assert(bytecode_function.code[15] == 0);

  beet_source_file_dispose(&file);
}

int main(void) {
  test_codegen_binding();
  test_codegen_return_const();
  test_codegen_return_local();
  test_codegen_lowered_function_return_local();
  test_codegen_lowered_arithmetic_return_expression();
  test_codegen_lowered_arithmetic_with_local_load();
  test_codegen_lowered_if_statement();
  test_codegen_lowered_while_statement();
  return 0;
}
