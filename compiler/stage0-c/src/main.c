#include <stdio.h>
#include <string.h>

#include "beet/codegen/codegen.h"
#include "beet/compiler/api.h"
#include "beet/lexer/token.h"
#include "beet/mir/mir.h"
#include "beet/parser/parser.h"
#include "beet/resolve/scope.h"
#include "beet/support/source.h"
#include "beet/types/check.h"
#include "beet/vm/interpreter.h"

#define BEET_DRIVER_MAX_TYPE_DECLS 32
#define BEET_DRIVER_MAX_FUNCTIONS 32

static int beet_name_equals(const char *name, size_t name_len,
                            const char *expected) {
  size_t expected_len;

  expected_len = strlen(expected);
  return expected_len == name_len && strncmp(name, expected, name_len) == 0;
}

static int beet_find_main_index(const beet_ast_function *functions,
                                size_t function_count, size_t *out_index) {
  size_t i;
  int found;

  found = 0;
  for (i = 0U; i < function_count; ++i) {
    if (!beet_name_equals(functions[i].name, functions[i].name_len, "main")) {
      continue;
    }

    if (found) {
      return 0;
    }

    *out_index = i;
    found = 1;
  }

  return found;
}

static int beet_compile_and_run_file(const char *path, int *out_result) {
  beet_source_file file;
  beet_parser parser;
  static beet_ast_type_decl type_decls[BEET_DRIVER_MAX_TYPE_DECLS];
  static beet_ast_function functions[BEET_DRIVER_MAX_FUNCTIONS];
  static beet_mir_function mir_functions[BEET_DRIVER_MAX_FUNCTIONS];
  static beet_bytecode_program bytecode_program;
  beet_vm vm;
  size_t type_decl_count;
  size_t function_count;
  size_t main_index;
  size_t i;

  beet_source_file_init(&file);

  if (!beet_source_file_load(&file, path)) {
    fprintf(stderr, "error: failed to load source file '%s'\n", path);
    return 0;
  }

  beet_parser_init(&parser, &file);
  type_decl_count = 0U;
  function_count = 0U;

  while (parser.current.kind == BEET_TOKEN_KW_TYPE) {
    if (type_decl_count >= BEET_DRIVER_MAX_TYPE_DECLS) {
      fprintf(stderr, "error: too many type declarations in '%s'\n", path);
      beet_source_file_dispose(&file);
      return 0;
    }

    if (!beet_parser_parse_type_decl(&parser, &type_decls[type_decl_count])) {
      fprintf(stderr, "error: failed to parse type declaration in '%s'\n",
              path);
      beet_source_file_dispose(&file);
      return 0;
    }

    type_decl_count += 1U;
  }

  while (parser.current.kind == BEET_TOKEN_KW_FUNCTION) {
    if (function_count >= BEET_DRIVER_MAX_FUNCTIONS) {
      fprintf(stderr, "error: too many top-level functions in '%s'\n", path);
      beet_source_file_dispose(&file);
      return 0;
    }

    if (!beet_parser_parse_function(&parser, &functions[function_count])) {
      fprintf(stderr, "error: failed to parse top-level function in '%s'\n",
              path);
      beet_source_file_dispose(&file);
      return 0;
    }

    function_count += 1U;
  }

  if (parser.current.kind != BEET_TOKEN_EOF) {
    fprintf(stderr, "error: unexpected top-level form in '%s'\n", path);
    beet_source_file_dispose(&file);
    return 0;
  }

  if (function_count == 0U) {
    fprintf(stderr, "error: expected at least one top-level function in '%s'\n",
            path);
    beet_source_file_dispose(&file);
    return 0;
  }

  if (!beet_find_main_index(functions, function_count, &main_index)) {
    fprintf(stderr,
            "error: expected exactly one top-level function 'main' in '%s'\n",
            path);
    beet_source_file_dispose(&file);
    return 0;
  }

  if (!beet_type_check_type_decls(type_decls, type_decl_count)) {
    fprintf(stderr, "error: invalid type declarations in '%s'\n", path);
    beet_source_file_dispose(&file);
    return 0;
  }

  for (i = 0U; i < function_count; ++i) {
    if (!beet_type_check_function_signature_with_type_decls(
            &functions[i], type_decls, type_decl_count)) {
      fprintf(stderr, "error: invalid function signature in '%s'\n", path);
      beet_source_file_dispose(&file);
      return 0;
    }
  }

  for (i = 0U; i < function_count; ++i) {
    if (!beet_resolve_function_with_decls(&functions[i], functions,
                                          function_count)) {
      fprintf(stderr, "error: failed to resolve function names in '%s'\n",
              path);
      beet_source_file_dispose(&file);
      return 0;
    }
  }

  for (i = 0U; i < function_count; ++i) {
    if (!beet_type_check_function_body_with_decls(&functions[i], type_decls,
                                                  type_decl_count, functions,
                                                  function_count)) {
      fprintf(stderr, "error: invalid function body types in '%s'\n", path);
      beet_source_file_dispose(&file);
      return 0;
    }
  }

  for (i = 0U; i < function_count; ++i) {
    if (!beet_mir_lower_function(&mir_functions[i], &functions[i])) {
      fprintf(stderr, "error: failed to lower function to MIR\n");
      beet_source_file_dispose(&file);
      return 0;
    }
  }

  if (!beet_codegen_program(mir_functions, function_count, &bytecode_program)) {
    fprintf(stderr, "error: failed to generate bytecode\n");
    beet_source_file_dispose(&file);
    return 0;
  }

  if (!beet_vm_execute_program(&vm, &bytecode_program, main_index,
                               out_result)) {
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
