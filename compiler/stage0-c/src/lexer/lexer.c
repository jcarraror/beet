#include "beet/lexer/lexer.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static bool beet_is_alpha(char ch) {
  return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_');
}

static bool beet_is_digit(char ch) { return (ch >= '0' && ch <= '9'); }

static bool beet_is_alnum(char ch) {
  return beet_is_alpha(ch) || beet_is_digit(ch);
}

static bool beet_is_space(char ch) {
  return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

static char beet_peek(const beet_lexer *lexer) {
  assert(lexer != NULL);
  assert(lexer->file != NULL);
  assert(lexer->file->text != NULL);

  if (lexer->offset >= lexer->file->text_len) {
    return '\0';
  }

  return lexer->file->text[lexer->offset];
}

static char beet_advance(beet_lexer *lexer) {
  char ch;

  assert(lexer != NULL);

  ch = beet_peek(lexer);
  if (ch != '\0') {
    lexer->offset += 1U;
  }

  return ch;
}

static void beet_skip_whitespace(beet_lexer *lexer) {
  while (beet_is_space(beet_peek(lexer))) {
    (void)beet_advance(lexer);
  }
}

static beet_token beet_make_token(const beet_lexer *lexer, beet_token_kind kind,
                                  size_t start, size_t end) {
  beet_token token;

  assert(lexer != NULL);
  assert(lexer->file != NULL);
  assert(start <= end);
  assert(end <= lexer->file->text_len);

  token.kind = kind;
  token.span = beet_source_span_from_offsets(lexer->file, start, end);
  token.lexeme = lexer->file->text + start;
  token.lexeme_len = end - start;
  return token;
}

static beet_token_kind beet_identifier_kind(const char *text, size_t len) {
  if (len == 4U && strncmp(text, "bind", 4U) == 0) {
    return BEET_TOKEN_KW_BIND;
  }
  if (len == 7U && strncmp(text, "mutable", 7U) == 0) {
    return BEET_TOKEN_KW_MUTABLE;
  }
  if (len == 8U && strncmp(text, "function", 8U) == 0) {
    return BEET_TOKEN_KW_FUNCTION;
  }
  if (len == 7U && strncmp(text, "returns", 7U) == 0) {
    return BEET_TOKEN_KW_RETURNS;
  }
  if (len == 6U && strncmp(text, "return", 6U) == 0) {
    return BEET_TOKEN_KW_RETURN;
  }
  if (len == 2U && strncmp(text, "is", 2U) == 0) {
    return BEET_TOKEN_KW_IS;
  }
  if (len == 4U && strncmp(text, "type", 4U) == 0) {
    return BEET_TOKEN_KW_TYPE;
  }
  if (len == 9U && strncmp(text, "structure", 9U) == 0) {
    return BEET_TOKEN_KW_STRUCTURE;
  }
  if (len == 6U && strncmp(text, "choice", 6U) == 0) {
    return BEET_TOKEN_KW_CHOICE;
  }
  if (len == 5U && strncmp(text, "match", 5U) == 0) {
    return BEET_TOKEN_KW_MATCH;
  }
  if (len == 4U && strncmp(text, "case", 4U) == 0) {
    return BEET_TOKEN_KW_CASE;
  }
  if (len == 2U && strncmp(text, "if", 2U) == 0) {
    return BEET_TOKEN_KW_IF;
  }
  if (len == 5U && strncmp(text, "while", 5U) == 0) {
    return BEET_TOKEN_KW_WHILE;
  }
  if (len == 8U && strncmp(text, "borrowed", 8U) == 0) {
    return BEET_TOKEN_KW_BORROWED;
  }
  if (len == 5U && strncmp(text, "owned", 5U) == 0) {
    return BEET_TOKEN_KW_OWNED;
  }

  return BEET_TOKEN_IDENTIFIER;
}

static beet_token beet_scan_identifier(beet_lexer *lexer, size_t start) {
  while (beet_is_alnum(beet_peek(lexer))) {
    (void)beet_advance(lexer);
  }

  return beet_make_token(
      lexer,
      beet_identifier_kind(lexer->file->text + start, lexer->offset - start),
      start, lexer->offset);
}

static beet_token beet_scan_int(beet_lexer *lexer, size_t start) {
  while (beet_is_digit(beet_peek(lexer))) {
    (void)beet_advance(lexer);
  }

  return beet_make_token(lexer, BEET_TOKEN_INT_LITERAL, start, lexer->offset);
}

const char *beet_token_kind_name(beet_token_kind kind) {
  switch (kind) {
  case BEET_TOKEN_ERROR:
    return "error";
  case BEET_TOKEN_EOF:
    return "eof";
  case BEET_TOKEN_IDENTIFIER:
    return "identifier";
  case BEET_TOKEN_INT_LITERAL:
    return "int_literal";
  case BEET_TOKEN_KW_BIND:
    return "kw_bind";
  case BEET_TOKEN_KW_MUTABLE:
    return "kw_mutable";
  case BEET_TOKEN_KW_FUNCTION:
    return "kw_function";
  case BEET_TOKEN_KW_RETURNS:
    return "kw_returns";
  case BEET_TOKEN_KW_RETURN:
    return "kw_return";
  case BEET_TOKEN_KW_IS:
    return "kw_is";
  case BEET_TOKEN_KW_TYPE:
    return "kw_type";
  case BEET_TOKEN_KW_STRUCTURE:
    return "kw_structure";
  case BEET_TOKEN_KW_CHOICE:
    return "kw_choice";
  case BEET_TOKEN_KW_MATCH:
    return "kw_match";
  case BEET_TOKEN_KW_CASE:
    return "kw_case";
  case BEET_TOKEN_KW_IF:
    return "kw_if";
  case BEET_TOKEN_KW_WHILE:
    return "kw_while";
  case BEET_TOKEN_KW_BORROWED:
    return "kw_borrowed";
  case BEET_TOKEN_KW_OWNED:
    return "kw_owned";
  case BEET_TOKEN_LPAREN:
    return "lparen";
  case BEET_TOKEN_RPAREN:
    return "rparen";
  case BEET_TOKEN_LBRACE:
    return "lbrace";
  case BEET_TOKEN_RBRACE:
    return "rbrace";
  case BEET_TOKEN_COMMA:
    return "comma";
  case BEET_TOKEN_DOT:
    return "dot";
  case BEET_TOKEN_EQUAL:
    return "equal";
  case BEET_TOKEN_EQUAL_EQUAL:
    return "equal_equal";
  case BEET_TOKEN_BANG_EQUAL:
    return "bang_equal";
  case BEET_TOKEN_LESS:
    return "less";
  case BEET_TOKEN_LESS_EQUAL:
    return "less_equal";
  case BEET_TOKEN_GREATER:
    return "greater";
  case BEET_TOKEN_GREATER_EQUAL:
    return "greater_equal";
  case BEET_TOKEN_PLUS:
    return "plus";
  case BEET_TOKEN_MINUS:
    return "minus";
  case BEET_TOKEN_STAR:
    return "star";
  case BEET_TOKEN_SLASH:
    return "slash";
  }

  return "unknown";
}

void beet_lexer_init(beet_lexer *lexer, const beet_source_file *file) {
  assert(lexer != NULL);
  assert(file != NULL);
  assert(file->text != NULL);

  lexer->file = file;
  lexer->offset = 0U;
}

beet_token beet_lexer_next(beet_lexer *lexer) {
  size_t start;
  char ch;

  assert(lexer != NULL);
  assert(lexer->file != NULL);
  assert(lexer->file->text != NULL);

  beet_skip_whitespace(lexer);

  start = lexer->offset;
  ch = beet_peek(lexer);

  if (ch == '\0') {
    return beet_make_token(lexer, BEET_TOKEN_EOF, start, start);
  }

  if (beet_is_alpha(ch)) {
    (void)beet_advance(lexer);
    return beet_scan_identifier(lexer, start);
  }

  if (beet_is_digit(ch)) {
    (void)beet_advance(lexer);
    return beet_scan_int(lexer, start);
  }

  (void)beet_advance(lexer);

  switch (ch) {
  case '(':
    return beet_make_token(lexer, BEET_TOKEN_LPAREN, start, lexer->offset);
  case ')':
    return beet_make_token(lexer, BEET_TOKEN_RPAREN, start, lexer->offset);
  case '{':
    return beet_make_token(lexer, BEET_TOKEN_LBRACE, start, lexer->offset);
  case '}':
    return beet_make_token(lexer, BEET_TOKEN_RBRACE, start, lexer->offset);
  case ',':
    return beet_make_token(lexer, BEET_TOKEN_COMMA, start, lexer->offset);
  case '.':
    return beet_make_token(lexer, BEET_TOKEN_DOT, start, lexer->offset);
  case '=':
    if (beet_peek(lexer) == '=') {
      (void)beet_advance(lexer);
      return beet_make_token(lexer, BEET_TOKEN_EQUAL_EQUAL, start,
                             lexer->offset);
    }
    return beet_make_token(lexer, BEET_TOKEN_EQUAL, start, lexer->offset);
  case '!':
    if (beet_peek(lexer) == '=') {
      (void)beet_advance(lexer);
      return beet_make_token(lexer, BEET_TOKEN_BANG_EQUAL, start,
                             lexer->offset);
    }
    return beet_make_token(lexer, BEET_TOKEN_ERROR, start, lexer->offset);
  case '<':
    if (beet_peek(lexer) == '=') {
      (void)beet_advance(lexer);
      return beet_make_token(lexer, BEET_TOKEN_LESS_EQUAL, start,
                             lexer->offset);
    }
    return beet_make_token(lexer, BEET_TOKEN_LESS, start, lexer->offset);
  case '>':
    if (beet_peek(lexer) == '=') {
      (void)beet_advance(lexer);
      return beet_make_token(lexer, BEET_TOKEN_GREATER_EQUAL, start,
                             lexer->offset);
    }
    return beet_make_token(lexer, BEET_TOKEN_GREATER, start, lexer->offset);
  case '+':
    return beet_make_token(lexer, BEET_TOKEN_PLUS, start, lexer->offset);
  case '-':
    return beet_make_token(lexer, BEET_TOKEN_MINUS, start, lexer->offset);
  case '*':
    return beet_make_token(lexer, BEET_TOKEN_STAR, start, lexer->offset);
  case '/':
    return beet_make_token(lexer, BEET_TOKEN_SLASH, start, lexer->offset);
  default:
    return beet_make_token(lexer, BEET_TOKEN_ERROR, start, lexer->offset);
  }
}