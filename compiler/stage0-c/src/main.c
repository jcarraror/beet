#include <stdio.h>
#include <string.h>

#include "beet/codegen/codegen.h"
#include "beet/compiler/api.h"
#include "beet/mir/mir.h"
#include "beet/parser/parser.h"
#include "beet/support/source.h"
#include "beet/types/check.h"
#include "beet/vm/interpreter.h"

static int beet_name_equals(const char *name, size_t name_len,
                            const char *expected) {
  size_t expected_len;

  expected_len = strlen(expected);
  return expected_len == name_len && strncmp(name, expected, name_len) == 0;
}

static int beet_compile_and_run_file(const char *path, int *out_result) {
  beet_source_file file;
  beet_parser parser;
  beet_ast_function function_ast;
  beet_mir_function mir_function;
  beet_bytecode_function bytecode_function;
  beet_vm vm;

  beet_source_file_init(&file);

  if (!beet_source_file_load(&file, path)) {
    fprintf(stderr, "error: failed to load source file '%s'\n", path);
    return 0;
  }

  beet_parser_init(&parser, &file);

  if (!beet_parser_parse_function(&parser, &function_ast)) {
    fprintf(stderr, "error: failed to parse top-level function in '%s'\n",
            path);
    beet_source_file_dispose(&file);
    return 0;
  }

  if (!beet_name_equals(function_ast.name, function_ast.name_len, "main")) {
    fprintf(stderr, "error: expected top-level function 'main' in '%s'\n",
            path);
    beet_source_file_dispose(&file);
    return 0;
  }

  if (!beet_type_check_function_signature(&function_ast)) {
    fprintf(stderr, "error: invalid function signature in '%s'\n", path);
    beet_source_file_dispose(&file);
    return 0;
  }

  if (!function_ast.has_trivial_return_const_int) {
    fprintf(stderr,
            "error: only trivial integer returns are supported in '%s'\n",
            path);
    beet_source_file_dispose(&file);
    return 0;
  }

  if (!beet_mir_lower_trivial_return_function(
          &mir_function, &function_ast,
          function_ast.trivial_return_const_int)) {
    fprintf(stderr, "error: failed to lower function to MIR\n");
    beet_source_file_dispose(&file);
    return 0;
  }

  if (!beet_codegen_function(&mir_function, &bytecode_function)) {
    fprintf(stderr, "error: failed to generate bytecode\n");
    beet_source_file_dispose(&file);
    return 0;
  }

  if (!beet_vm_execute(&vm, &bytecode_function, out_result)) {
    fprintf(stderr, "error: VM execution failed\n");
    beet_source_file_dispose(&file);
    return 0;
  }

  beet_source_file_dispose(&file);
  return 1;
}

int main(int argc, char **argv) {
  int result;

  if (argc != 2) {
    fprintf(stderr, "%s: usage: %s <file.beet>\n", BEET_COMPILER_NAME, argv[0]);
    return 1;
  }

  if (!beet_compile_and_run_file(argv[1], &result)) {
    return 1;
  }

  printf("%d\n", result);
  return 0;
}