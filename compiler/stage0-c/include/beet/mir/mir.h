#ifndef BEET_MIR_MIR_H
#define BEET_MIR_MIR_H

#include <stddef.h>

#include "beet/parser/ast.h"

#define BEET_MIR_MAX_INSTRS 256
#define BEET_MIR_MAX_LOCALS 128
#define BEET_MIR_MAX_NAME_LEN 64

typedef enum beet_mir_opcode {
  BEET_MIR_OP_CONST_INT = 0,
  BEET_MIR_OP_BIND_LOCAL,
  BEET_MIR_OP_RETURN_LOCAL,
  BEET_MIR_OP_RETURN_CONST_INT
} beet_mir_opcode;

typedef struct beet_mir_instr {
  beet_mir_opcode op;
  int dst;
  int int_value;
  char name[BEET_MIR_MAX_NAME_LEN];
} beet_mir_instr;

typedef struct beet_mir_function {
  char name[BEET_MIR_MAX_NAME_LEN];
  beet_mir_instr instrs[BEET_MIR_MAX_INSTRS];
  size_t instr_count;

  char locals[BEET_MIR_MAX_LOCALS][BEET_MIR_MAX_NAME_LEN];
  size_t local_count;

  int next_temp;
} beet_mir_function;

void beet_mir_function_init(beet_mir_function *function, const char *name,
                            size_t name_len);

int beet_mir_add_const_int(beet_mir_function *function, int value);
int beet_mir_add_bind_local(beet_mir_function *function, const char *name,
                            size_t name_len, int src_temp);
int beet_mir_add_return_local(beet_mir_function *function, const char *name,
                              size_t name_len);
int beet_mir_add_return_const_int(beet_mir_function *function, int value);

int beet_mir_lower_binding(beet_mir_function *function,
                           const beet_ast_binding *binding);

int beet_mir_lower_trivial_return_function(
    beet_mir_function *function, const beet_ast_function *function_ast,
    size_t entry_index);

int beet_mir_lower_function(beet_mir_function *function,
                            const beet_ast_function *function_ast);

#endif