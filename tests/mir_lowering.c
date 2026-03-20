#include <assert.h>
#include <string.h>

#include "beet/mir/mir.h"
#include "beet/parser/parser.h"
#include "beet/support/source.h"

static void test_lower_binding_to_mir(void) {
  const char *text = "bind x = 10\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;
  beet_mir_function function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));

  beet_mir_function_init(&function, "main", 4U);
  assert(beet_mir_lower_binding(&function, &binding));

  assert(function.instr_count == 2U);

  assert(function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(function.instrs[0].dst == 0);
  assert(function.instrs[0].int_value == 10);

  assert(function.instrs[1].op == BEET_MIR_OP_BIND_LOCAL);
  assert(function.instrs[1].dst == 0);
  assert(strcmp(function.instrs[1].name, "x") == 0);

  assert(function.local_count == 1U);
  assert(strcmp(function.locals[0], "x") == 0);

  beet_source_file_dispose(&file);
}

static void test_lower_mutable_binding_to_mir(void) {
  const char *text = "mutable total = 25\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_binding binding;
  beet_mir_function function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_binding(&parser, &binding));

  beet_mir_function_init(&function, "main", 4U);
  assert(beet_mir_lower_binding(&function, &binding));

  assert(function.instr_count == 2U);
  assert(function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(function.instrs[0].int_value == 25);
  assert(function.instrs[1].op == BEET_MIR_OP_BIND_LOCAL);
  assert(strcmp(function.instrs[1].name, "total") == 0);

  beet_source_file_dispose(&file);
}

static void test_lower_trivial_function_return(void) {
  const char *text = "function main() returns Int {\n"
                     "    return 0\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(
      beet_mir_lower_trivial_return_function(&mir_function, &function_ast, 0));

  assert(strcmp(mir_function.name, "main") == 0);
  assert(mir_function.instr_count == 1U);
  assert(mir_function.instrs[0].op == BEET_MIR_OP_RETURN_CONST_INT);
  assert(mir_function.instrs[0].int_value == 0);

  beet_source_file_dispose(&file);
}

static void test_lower_function_body_binding_and_return(void) {
  const char *text = "function main() returns Int {\n"
                     "    bind x = 10\n"
                     "    return x\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));

  assert(strcmp(mir_function.name, "main") == 0);
  assert(mir_function.instr_count == 3U);

  assert(mir_function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[0].dst == 0);
  assert(mir_function.instrs[0].int_value == 10);

  assert(mir_function.instrs[1].op == BEET_MIR_OP_BIND_LOCAL);
  assert(mir_function.instrs[1].dst == 0);
  assert(strcmp(mir_function.instrs[1].name, "x") == 0);

  assert(mir_function.instrs[2].op == BEET_MIR_OP_RETURN_LOCAL);
  assert(strcmp(mir_function.instrs[2].name, "x") == 0);

  assert(mir_function.local_count == 1U);
  assert(strcmp(mir_function.locals[0], "x") == 0);

  beet_source_file_dispose(&file);
}

int main(void) {
  test_lower_binding_to_mir();
  test_lower_mutable_binding_to_mir();
  test_lower_trivial_function_return();
  test_lower_function_body_binding_and_return();
  return 0;
}