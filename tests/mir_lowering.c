#include <assert.h>
#include <string.h>

#include "beet/mir/mir.h"
#include "beet/parser/parser.h"
#include "beet/resolve/scope.h"
#include "beet/support/source.h"
#include "beet/types/check.h"

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

static void test_lower_arithmetic_return_expression(void) {
  const char *text = "function main() returns Int {\n"
                     "    return 1 + 2 * 3\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));

  assert(mir_function.instr_count == 6U);

  assert(mir_function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[0].dst == 0);
  assert(mir_function.instrs[0].int_value == 1);

  assert(mir_function.instrs[1].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[1].dst == 1);
  assert(mir_function.instrs[1].int_value == 2);

  assert(mir_function.instrs[2].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[2].dst == 2);
  assert(mir_function.instrs[2].int_value == 3);

  assert(mir_function.instrs[3].op == BEET_MIR_OP_MUL_INT);
  assert(mir_function.instrs[3].dst == 3);
  assert(mir_function.instrs[3].src_lhs == 1);
  assert(mir_function.instrs[3].src_rhs == 2);

  assert(mir_function.instrs[4].op == BEET_MIR_OP_ADD_INT);
  assert(mir_function.instrs[4].dst == 4);
  assert(mir_function.instrs[4].src_lhs == 0);
  assert(mir_function.instrs[4].src_rhs == 3);

  assert(mir_function.instrs[5].op == BEET_MIR_OP_RETURN_TEMP);
  assert(mir_function.instrs[5].dst == 4);

  beet_source_file_dispose(&file);
}

static void test_lower_unary_arithmetic_return_expression(void) {
  const char *text = "function main() returns Int {\n"
                     "    return -(1 + 2)\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));

  assert(mir_function.instr_count == 6U);

  assert(mir_function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[0].dst == 0);
  assert(mir_function.instrs[0].int_value == 0);

  assert(mir_function.instrs[1].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[1].dst == 1);
  assert(mir_function.instrs[1].int_value == 1);

  assert(mir_function.instrs[2].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[2].dst == 2);
  assert(mir_function.instrs[2].int_value == 2);

  assert(mir_function.instrs[3].op == BEET_MIR_OP_ADD_INT);
  assert(mir_function.instrs[3].dst == 3);
  assert(mir_function.instrs[3].src_lhs == 1);
  assert(mir_function.instrs[3].src_rhs == 2);

  assert(mir_function.instrs[4].op == BEET_MIR_OP_SUB_INT);
  assert(mir_function.instrs[4].dst == 4);
  assert(mir_function.instrs[4].src_lhs == 0);
  assert(mir_function.instrs[4].src_rhs == 3);

  assert(mir_function.instrs[5].op == BEET_MIR_OP_RETURN_TEMP);
  assert(mir_function.instrs[5].dst == 4);

  beet_source_file_dispose(&file);
}

static void test_lower_arithmetic_with_local_load(void) {
  const char *text = "function main() returns Int {\n"
                     "    bind x = 10\n"
                     "    return x + 2\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));

  assert(mir_function.instr_count == 6U);
  assert(mir_function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[1].op == BEET_MIR_OP_BIND_LOCAL);
  assert(mir_function.instrs[2].op == BEET_MIR_OP_LOAD_LOCAL);
  assert(strcmp(mir_function.instrs[2].name, "x") == 0);
  assert(mir_function.instrs[3].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[3].int_value == 2);
  assert(mir_function.instrs[4].op == BEET_MIR_OP_ADD_INT);
  assert(mir_function.instrs[4].src_lhs == 1);
  assert(mir_function.instrs[4].src_rhs == 2);
  assert(mir_function.instrs[5].op == BEET_MIR_OP_RETURN_TEMP);
  assert(mir_function.instrs[5].dst == 3);

  beet_source_file_dispose(&file);
}

static void test_lower_comparison_return_expression(void) {
  const char *text = "function main() returns Bool {\n"
                     "    return 1 < 2\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));

  assert(mir_function.instr_count == 4U);
  assert(mir_function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[0].dst == 0);
  assert(mir_function.instrs[0].int_value == 1);
  assert(mir_function.instrs[1].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[1].dst == 1);
  assert(mir_function.instrs[1].int_value == 2);
  assert(mir_function.instrs[2].op == BEET_MIR_OP_LT_INT);
  assert(mir_function.instrs[2].dst == 2);
  assert(mir_function.instrs[2].src_lhs == 0);
  assert(mir_function.instrs[2].src_rhs == 1);
  assert(mir_function.instrs[3].op == BEET_MIR_OP_RETURN_TEMP);
  assert(mir_function.instrs[3].dst == 2);

  beet_source_file_dispose(&file);
}

static void test_lower_while_statement_with_bool_literal(void) {
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

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));

  assert(strcmp(mir_function.name, "main") == 0);
  assert(mir_function.instr_count == 7U);
  assert(mir_function.instrs[0].op == BEET_MIR_OP_LABEL);
  assert(mir_function.instrs[0].int_value == 0);
  assert(mir_function.instrs[1].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[1].dst == 0);
  assert(mir_function.instrs[1].int_value == 1);
  assert(mir_function.instrs[2].op == BEET_MIR_OP_JUMP_IF_FALSE);
  assert(mir_function.instrs[2].dst == 0);
  assert(mir_function.instrs[2].int_value == 1);
  assert(mir_function.instrs[3].op == BEET_MIR_OP_RETURN_CONST_INT);
  assert(mir_function.instrs[3].int_value == 1);
  assert(mir_function.instrs[4].op == BEET_MIR_OP_JUMP);
  assert(mir_function.instrs[4].int_value == 0);
  assert(mir_function.instrs[5].op == BEET_MIR_OP_LABEL);
  assert(mir_function.instrs[5].int_value == 1);
  assert(mir_function.instrs[6].op == BEET_MIR_OP_RETURN_CONST_INT);
  assert(mir_function.instrs[6].int_value == 0);

  beet_source_file_dispose(&file);
}

static void test_lower_if_statement_with_bool_literal(void) {
  const char *text = "function main() returns Int {\n"
                     "    if true {\n"
                     "        return 1\n"
                     "    }\n"
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
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));

  assert(strcmp(mir_function.name, "main") == 0);
  assert(mir_function.instr_count == 5U);
  assert(mir_function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[0].dst == 0);
  assert(mir_function.instrs[0].int_value == 1);
  assert(mir_function.instrs[1].op == BEET_MIR_OP_JUMP_IF_FALSE);
  assert(mir_function.instrs[1].dst == 0);
  assert(mir_function.instrs[1].int_value == 0);
  assert(mir_function.instrs[2].op == BEET_MIR_OP_RETURN_CONST_INT);
  assert(mir_function.instrs[2].int_value == 1);
  assert(mir_function.instrs[3].op == BEET_MIR_OP_LABEL);
  assert(mir_function.instrs[3].int_value == 0);
  assert(mir_function.instrs[4].op == BEET_MIR_OP_RETURN_CONST_INT);
  assert(mir_function.instrs[4].int_value == 0);

  beet_source_file_dispose(&file);
}

static void test_lower_if_statement_with_bool_param(void) {
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

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));

  assert(strcmp(mir_function.name, "choose") == 0);
  assert(mir_function.instr_count == 5U);

  assert(mir_function.instrs[0].op == BEET_MIR_OP_LOAD_LOCAL);
  assert(mir_function.instrs[0].dst == 0);
  assert(strcmp(mir_function.instrs[0].name, "flag") == 0);

  assert(mir_function.instrs[1].op == BEET_MIR_OP_JUMP_IF_FALSE);
  assert(mir_function.instrs[1].dst == 0);
  assert(mir_function.instrs[1].int_value == 0);

  assert(mir_function.instrs[2].op == BEET_MIR_OP_RETURN_CONST_INT);
  assert(mir_function.instrs[2].int_value == 1);

  assert(mir_function.instrs[3].op == BEET_MIR_OP_LABEL);
  assert(mir_function.instrs[3].int_value == 0);

  assert(mir_function.instrs[4].op == BEET_MIR_OP_RETURN_CONST_INT);
  assert(mir_function.instrs[4].int_value == 0);

  beet_source_file_dispose(&file);
}

static void test_lower_if_else_statement_with_bool_param(void) {
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
  beet_mir_function mir_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));

  assert(strcmp(mir_function.name, "choose") == 0);
  assert(mir_function.instr_count == 7U);

  assert(mir_function.instrs[0].op == BEET_MIR_OP_LOAD_LOCAL);
  assert(mir_function.instrs[0].dst == 0);
  assert(strcmp(mir_function.instrs[0].name, "flag") == 0);

  assert(mir_function.instrs[1].op == BEET_MIR_OP_JUMP_IF_FALSE);
  assert(mir_function.instrs[1].dst == 0);
  assert(mir_function.instrs[1].int_value == 0);

  assert(mir_function.instrs[2].op == BEET_MIR_OP_RETURN_CONST_INT);
  assert(mir_function.instrs[2].int_value == 1);

  assert(mir_function.instrs[3].op == BEET_MIR_OP_JUMP);
  assert(mir_function.instrs[3].int_value == 1);

  assert(mir_function.instrs[4].op == BEET_MIR_OP_LABEL);
  assert(mir_function.instrs[4].int_value == 0);

  assert(mir_function.instrs[5].op == BEET_MIR_OP_RETURN_CONST_INT);
  assert(mir_function.instrs[5].int_value == 2);

  assert(mir_function.instrs[6].op == BEET_MIR_OP_LABEL);
  assert(mir_function.instrs[6].int_value == 1);

  beet_source_file_dispose(&file);
}

static void test_lower_assignment_inside_if_statement(void) {
  const char *text = "function main() returns Int {\n"
                     "    mutable total = 1\n"
                     "    if true {\n"
                     "        total = total + 2\n"
                     "    }\n"
                     "    return total\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &function_ast));
  assert(beet_resolve_function(&function_ast));
  assert(beet_type_check_function_signature(&function_ast));
  assert(beet_type_check_function_body(&function_ast));
  assert(beet_mir_lower_function(&mir_function, &function_ast));

  assert(strcmp(mir_function.name, "main") == 0);
  assert(mir_function.instr_count == 10U);
  assert(mir_function.local_count == 1U);
  assert(strcmp(mir_function.locals[0], "total") == 0);

  assert(mir_function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[0].dst == 0);
  assert(mir_function.instrs[0].int_value == 1);

  assert(mir_function.instrs[1].op == BEET_MIR_OP_BIND_LOCAL);
  assert(mir_function.instrs[1].dst == 0);
  assert(strcmp(mir_function.instrs[1].name, "total") == 0);

  assert(mir_function.instrs[2].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[2].dst == 1);
  assert(mir_function.instrs[2].int_value == 1);

  assert(mir_function.instrs[3].op == BEET_MIR_OP_JUMP_IF_FALSE);
  assert(mir_function.instrs[3].dst == 1);
  assert(mir_function.instrs[3].int_value == 0);

  assert(mir_function.instrs[4].op == BEET_MIR_OP_LOAD_LOCAL);
  assert(mir_function.instrs[4].dst == 2);
  assert(strcmp(mir_function.instrs[4].name, "total") == 0);

  assert(mir_function.instrs[5].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[5].dst == 3);
  assert(mir_function.instrs[5].int_value == 2);

  assert(mir_function.instrs[6].op == BEET_MIR_OP_ADD_INT);
  assert(mir_function.instrs[6].dst == 4);
  assert(mir_function.instrs[6].src_lhs == 2);
  assert(mir_function.instrs[6].src_rhs == 3);

  assert(mir_function.instrs[7].op == BEET_MIR_OP_STORE_LOCAL);
  assert(mir_function.instrs[7].dst == 4);
  assert(strcmp(mir_function.instrs[7].name, "total") == 0);

  assert(mir_function.instrs[8].op == BEET_MIR_OP_LABEL);
  assert(mir_function.instrs[8].int_value == 0);

  assert(mir_function.instrs[9].op == BEET_MIR_OP_RETURN_LOCAL);
  assert(strcmp(mir_function.instrs[9].name, "total") == 0);

  beet_source_file_dispose(&file);
}

static void test_lower_expression_binding_to_mir(void) {
  const char *text = "bind total = 1 + 2 * 3\n";
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

  assert(function.instr_count == 6U);
  assert(function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(function.instrs[0].int_value == 1);
  assert(function.instrs[1].op == BEET_MIR_OP_CONST_INT);
  assert(function.instrs[1].int_value == 2);
  assert(function.instrs[2].op == BEET_MIR_OP_CONST_INT);
  assert(function.instrs[2].int_value == 3);
  assert(function.instrs[3].op == BEET_MIR_OP_MUL_INT);
  assert(function.instrs[4].op == BEET_MIR_OP_ADD_INT);
  assert(function.instrs[5].op == BEET_MIR_OP_BIND_LOCAL);
  assert(strcmp(function.instrs[5].name, "total") == 0);
  assert(function.local_count == 1U);
  assert(strcmp(function.locals[0], "total") == 0);

  beet_source_file_dispose(&file);
}

static void test_lower_structure_binding_and_field_return(void) {
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

  assert(mir_function.local_count == 2U);
  assert(strcmp(mir_function.locals[0], "point.x") == 0);
  assert(strcmp(mir_function.locals[1], "point.y") == 0);
  assert(mir_function.instr_count == 6U);
  assert(mir_function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[0].int_value == 3);
  assert(mir_function.instrs[1].op == BEET_MIR_OP_BIND_LOCAL);
  assert(strcmp(mir_function.instrs[1].name, "point.x") == 0);
  assert(mir_function.instrs[2].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[2].int_value == 4);
  assert(mir_function.instrs[3].op == BEET_MIR_OP_BIND_LOCAL);
  assert(strcmp(mir_function.instrs[3].name, "point.y") == 0);
  assert(mir_function.instrs[4].op == BEET_MIR_OP_LOAD_LOCAL);
  assert(strcmp(mir_function.instrs[4].name, "point.x") == 0);
  assert(mir_function.instrs[5].op == BEET_MIR_OP_RETURN_TEMP);
  assert(mir_function.instrs[5].dst == 2);

  beet_source_file_dispose(&function_file);
  beet_source_file_dispose(&decl_file);
}

static void test_lower_function_call_expression(void) {
  const char *text = "function add1(x is Int) returns Int {\n"
                     "    return x + 1\n"
                     "}\n"
                     "function main() returns Int {\n"
                     "    return add1(41)\n"
                     "}\n";
  beet_source_file file;
  beet_parser parser;
  beet_ast_function functions[2];
  beet_mir_function mir_function;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, "test.beet", text));
  beet_parser_init(&parser, &file);

  assert(beet_parser_parse_function(&parser, &functions[0]));
  assert(beet_parser_parse_function(&parser, &functions[1]));
  assert(beet_resolve_function_with_decls(&functions[0], functions, 2U));
  assert(beet_resolve_function_with_decls(&functions[1], functions, 2U));
  assert(beet_type_check_function_signature(&functions[0]));
  assert(beet_type_check_function_signature(&functions[1]));
  assert(beet_type_check_function_body_with_decls(&functions[0], NULL, 0U,
                                                  functions, 2U));
  assert(beet_type_check_function_body_with_decls(&functions[1], NULL, 0U,
                                                  functions, 2U));
  assert(beet_mir_lower_function(&mir_function, &functions[1]));

  assert(mir_function.instr_count == 3U);
  assert(mir_function.instrs[0].op == BEET_MIR_OP_CONST_INT);
  assert(mir_function.instrs[0].dst == 0);
  assert(mir_function.instrs[0].int_value == 41);
  assert(mir_function.instrs[1].op == BEET_MIR_OP_CALL);
  assert(mir_function.instrs[1].dst == 1);
  assert(strcmp(mir_function.instrs[1].name, "add1") == 0);
  assert(mir_function.instrs[1].arg_count == 1U);
  assert(mir_function.instrs[1].args[0] == 0);
  assert(mir_function.instrs[2].op == BEET_MIR_OP_RETURN_TEMP);
  assert(mir_function.instrs[2].dst == 1);

  beet_source_file_dispose(&file);
}

int main(void) {
  test_lower_binding_to_mir();
  test_lower_mutable_binding_to_mir();
  test_lower_expression_binding_to_mir();
  test_lower_trivial_function_return();
  test_lower_function_body_binding_and_return();
  test_lower_arithmetic_return_expression();
  test_lower_unary_arithmetic_return_expression();
  test_lower_arithmetic_with_local_load();
  test_lower_comparison_return_expression();
  test_lower_while_statement_with_bool_literal();
  test_lower_if_statement_with_bool_literal();
  test_lower_if_statement_with_bool_param();
  test_lower_if_else_statement_with_bool_param();
  test_lower_assignment_inside_if_statement();
  test_lower_structure_binding_and_field_return();
  test_lower_function_call_expression();
  return 0;
}
