#include <assert.h>
#include <string.h>

#include "beet/codegen/codegen.h"
#include "beet/mir/mir.h"
#include "beet/parser/parser.h"
#include "beet/support/source.h"
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

int main(void) {
  test_codegen_binding();
  test_codegen_return_const();
  test_codegen_return_local();
  return 0;
}