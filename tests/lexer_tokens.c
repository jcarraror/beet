#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "beet/lexer/lexer.h"
#include "beet/lexer/token.h"
#include "beet/support/source.h"

typedef struct expected_token {
  beet_token_kind kind;
  const char *lexeme;
} expected_token;

static void check_tokens(const char *path, const char *text,
                         const expected_token *expected,
                         size_t expected_count) {
  beet_source_file file;
  beet_lexer lexer;
  beet_token token;
  size_t i;

  beet_source_file_init(&file);
  assert(beet_source_file_set_text_copy(&file, path, text));

  beet_lexer_init(&lexer, &file);

  for (i = 0U; i < expected_count; ++i) {
    token = beet_lexer_next(&lexer);
    assert(token.kind == expected[i].kind);
    assert(token.lexeme_len == strlen(expected[i].lexeme));
    assert(strncmp(token.lexeme, expected[i].lexeme, token.lexeme_len) == 0);
  }

  token = beet_lexer_next(&lexer);
  assert(token.kind == BEET_TOKEN_EOF);
  assert(token.lexeme_len == 0U);

  beet_source_file_dispose(&file);
}

static void test_binding_and_function_tokens(void) {
  static const char *text = "function main() returns Int {\n"
                            "    bind x = 1\n"
                            "    mutable total = 0\n"
                            "    return x\n"
                            "}\n";

  static const expected_token expected[] = {
      {BEET_TOKEN_KW_FUNCTION, "function"},
      {BEET_TOKEN_IDENTIFIER, "main"},
      {BEET_TOKEN_LPAREN, "("},
      {BEET_TOKEN_RPAREN, ")"},
      {BEET_TOKEN_KW_RETURNS, "returns"},
      {BEET_TOKEN_IDENTIFIER, "Int"},
      {BEET_TOKEN_LBRACE, "{"},
      {BEET_TOKEN_KW_BIND, "bind"},
      {BEET_TOKEN_IDENTIFIER, "x"},
      {BEET_TOKEN_EQUAL, "="},
      {BEET_TOKEN_INT_LITERAL, "1"},
      {BEET_TOKEN_KW_MUTABLE, "mutable"},
      {BEET_TOKEN_IDENTIFIER, "total"},
      {BEET_TOKEN_EQUAL, "="},
      {BEET_TOKEN_INT_LITERAL, "0"},
      {BEET_TOKEN_KW_RETURN, "return"},
      {BEET_TOKEN_IDENTIFIER, "x"},
      {BEET_TOKEN_RBRACE, "}"}};

  check_tokens("binding_function.beet", text, expected,
               sizeof(expected) / sizeof(expected[0]));
}

static void test_type_and_structure_tokens(void) {
  static const char *text = "type Point = structure {\n"
                            "    x is Int\n"
                            "    y is Int\n"
                            "}\n";

  static const expected_token expected[] = {
      {BEET_TOKEN_KW_TYPE, "type"},   {BEET_TOKEN_IDENTIFIER, "Point"},
      {BEET_TOKEN_EQUAL, "="},        {BEET_TOKEN_KW_STRUCTURE, "structure"},
      {BEET_TOKEN_LBRACE, "{"},       {BEET_TOKEN_IDENTIFIER, "x"},
      {BEET_TOKEN_KW_IS, "is"},       {BEET_TOKEN_IDENTIFIER, "Int"},
      {BEET_TOKEN_IDENTIFIER, "y"},   {BEET_TOKEN_KW_IS, "is"},
      {BEET_TOKEN_IDENTIFIER, "Int"}, {BEET_TOKEN_RBRACE, "}"}};

  check_tokens("type_structure.beet", text, expected,
               sizeof(expected) / sizeof(expected[0]));
}

static void test_choice_and_construction_tokens(void) {
  static const char *text = "type Option(Value) = choice {\n"
                            "    none\n"
                            "    some(Value)\n"
                            "}\n"
                            "bind point = Point(x = 3, y = 4)\n";

  static const expected_token expected[] = {
      {BEET_TOKEN_KW_TYPE, "type"},     {BEET_TOKEN_IDENTIFIER, "Option"},
      {BEET_TOKEN_LPAREN, "("},         {BEET_TOKEN_IDENTIFIER, "Value"},
      {BEET_TOKEN_RPAREN, ")"},         {BEET_TOKEN_EQUAL, "="},
      {BEET_TOKEN_KW_CHOICE, "choice"}, {BEET_TOKEN_LBRACE, "{"},
      {BEET_TOKEN_IDENTIFIER, "none"},  {BEET_TOKEN_IDENTIFIER, "some"},
      {BEET_TOKEN_LPAREN, "("},         {BEET_TOKEN_IDENTIFIER, "Value"},
      {BEET_TOKEN_RPAREN, ")"},         {BEET_TOKEN_RBRACE, "}"},
      {BEET_TOKEN_KW_BIND, "bind"},     {BEET_TOKEN_IDENTIFIER, "point"},
      {BEET_TOKEN_EQUAL, "="},          {BEET_TOKEN_IDENTIFIER, "Point"},
      {BEET_TOKEN_LPAREN, "("},         {BEET_TOKEN_IDENTIFIER, "x"},
      {BEET_TOKEN_EQUAL, "="},          {BEET_TOKEN_INT_LITERAL, "3"},
      {BEET_TOKEN_COMMA, ","},          {BEET_TOKEN_IDENTIFIER, "y"},
      {BEET_TOKEN_EQUAL, "="},          {BEET_TOKEN_INT_LITERAL, "4"},
      {BEET_TOKEN_RPAREN, ")"}};

  check_tokens("choice_construction.beet", text, expected,
               sizeof(expected) / sizeof(expected[0]));
}

static void test_unknown_character_yields_error(void) {
  static const char *text = "bind x = @\n";
  static const expected_token expected[] = {{BEET_TOKEN_KW_BIND, "bind"},
                                            {BEET_TOKEN_IDENTIFIER, "x"},
                                            {BEET_TOKEN_EQUAL, "="},
                                            {BEET_TOKEN_ERROR, "@"}};

  check_tokens("unknown_char.beet", text, expected,
               sizeof(expected) / sizeof(expected[0]));
}

static void test_control_flow_keyword_tokens(void) {
  static const char *text = "function main() returns Int {\n"
                            "    while true {\n"
                            "        if false {\n"
                            "            return 1\n"
                            "        } else {\n"
                            "            return 2\n"
                            "        }\n"
                            "    }\n"
                            "    return 0\n"
                            "}\n";

  static const expected_token expected[] = {
      {BEET_TOKEN_KW_FUNCTION, "function"},
      {BEET_TOKEN_IDENTIFIER, "main"},
      {BEET_TOKEN_LPAREN, "("},
      {BEET_TOKEN_RPAREN, ")"},
      {BEET_TOKEN_KW_RETURNS, "returns"},
      {BEET_TOKEN_IDENTIFIER, "Int"},
      {BEET_TOKEN_LBRACE, "{"},
      {BEET_TOKEN_KW_WHILE, "while"},
      {BEET_TOKEN_IDENTIFIER, "true"},
      {BEET_TOKEN_LBRACE, "{"},
      {BEET_TOKEN_KW_IF, "if"},
      {BEET_TOKEN_IDENTIFIER, "false"},
      {BEET_TOKEN_LBRACE, "{"},
      {BEET_TOKEN_KW_RETURN, "return"},
      {BEET_TOKEN_INT_LITERAL, "1"},
      {BEET_TOKEN_RBRACE, "}"},
      {BEET_TOKEN_KW_ELSE, "else"},
      {BEET_TOKEN_LBRACE, "{"},
      {BEET_TOKEN_KW_RETURN, "return"},
      {BEET_TOKEN_INT_LITERAL, "2"},
      {BEET_TOKEN_RBRACE, "}"},
      {BEET_TOKEN_RBRACE, "}"},
      {BEET_TOKEN_KW_RETURN, "return"},
      {BEET_TOKEN_INT_LITERAL, "0"},
      {BEET_TOKEN_RBRACE, "}"}};

  check_tokens("control_flow_tokens.beet", text, expected,
               sizeof(expected) / sizeof(expected[0]));
}

static void test_arithmetic_operator_tokens(void) {
  static const char *text = "function main() returns Int {\n"
                            "    return -1 + 2 * 3 / 4\n"
                            "}\n";

  static const expected_token expected[] = {
      {BEET_TOKEN_KW_FUNCTION, "function"},
      {BEET_TOKEN_IDENTIFIER, "main"},
      {BEET_TOKEN_LPAREN, "("},
      {BEET_TOKEN_RPAREN, ")"},
      {BEET_TOKEN_KW_RETURNS, "returns"},
      {BEET_TOKEN_IDENTIFIER, "Int"},
      {BEET_TOKEN_LBRACE, "{"},
      {BEET_TOKEN_KW_RETURN, "return"},
      {BEET_TOKEN_MINUS, "-"},
      {BEET_TOKEN_INT_LITERAL, "1"},
      {BEET_TOKEN_PLUS, "+"},
      {BEET_TOKEN_INT_LITERAL, "2"},
      {BEET_TOKEN_STAR, "*"},
      {BEET_TOKEN_INT_LITERAL, "3"},
      {BEET_TOKEN_SLASH, "/"},
      {BEET_TOKEN_INT_LITERAL, "4"},
      {BEET_TOKEN_RBRACE, "}"}};

  check_tokens("arithmetic_tokens.beet", text, expected,
               sizeof(expected) / sizeof(expected[0]));
}

static void test_comparison_operator_tokens(void) {
  static const char *text = "function main() returns Bool {\n"
                            "    return 1 == 2 != 3 < 4 <= 5 > 6 >= 7\n"
                            "}\n";

  static const expected_token expected[] = {
      {BEET_TOKEN_KW_FUNCTION, "function"},
      {BEET_TOKEN_IDENTIFIER, "main"},
      {BEET_TOKEN_LPAREN, "("},
      {BEET_TOKEN_RPAREN, ")"},
      {BEET_TOKEN_KW_RETURNS, "returns"},
      {BEET_TOKEN_IDENTIFIER, "Bool"},
      {BEET_TOKEN_LBRACE, "{"},
      {BEET_TOKEN_KW_RETURN, "return"},
      {BEET_TOKEN_INT_LITERAL, "1"},
      {BEET_TOKEN_EQUAL_EQUAL, "=="},
      {BEET_TOKEN_INT_LITERAL, "2"},
      {BEET_TOKEN_BANG_EQUAL, "!="},
      {BEET_TOKEN_INT_LITERAL, "3"},
      {BEET_TOKEN_LESS, "<"},
      {BEET_TOKEN_INT_LITERAL, "4"},
      {BEET_TOKEN_LESS_EQUAL, "<="},
      {BEET_TOKEN_INT_LITERAL, "5"},
      {BEET_TOKEN_GREATER, ">"},
      {BEET_TOKEN_INT_LITERAL, "6"},
      {BEET_TOKEN_GREATER_EQUAL, ">="},
      {BEET_TOKEN_INT_LITERAL, "7"},
      {BEET_TOKEN_RBRACE, "}"}};

  check_tokens("comparison_tokens.beet", text, expected,
               sizeof(expected) / sizeof(expected[0]));
}

int main(void) {
  test_binding_and_function_tokens();
  test_type_and_structure_tokens();
  test_choice_and_construction_tokens();
  test_unknown_character_yields_error();
  test_control_flow_keyword_tokens();
  test_arithmetic_operator_tokens();
  test_comparison_operator_tokens();
  return 0;
}
