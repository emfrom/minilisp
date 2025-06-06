#include <assert.h>
#include <ctype.h>
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "numbers.h"
#include "parser.h"
#include "repl.h"
#include "value.h"

// TODO: Go over and make sure error state on fp is properly handled

/**
 * Minimalist lexer
 */
typedef enum {
  TOK_EOF,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_QUOTE,
  TOK_SYMBOL,
  TOK_NUMBER,
  TOK_BOOL,
  TOK_STRING,
  TOK_COMMENT
} tokenType;

typedef struct {
  tokenType type;
  char *text;
} token;

int peek(FILE *in) {
  int c = fgetc(in);

  if (c != EOF)
    ungetc(c, in);
  return c;
}

// Store for next token
static token tok = {0};

int token_pushed = 0;

void token_push(token to_push) {
  assert(!token_pushed);

  tok = to_push;
  token_pushed = 1;
}

// Temp buffer for token data
char buf[1024];

// Simple token reader
token token_getnext(FILE *in) {

  if (token_pushed) {
    token_pushed = 0;
    return tok;
  }

  // TODO: More robust
  tok.text = buf;

  // Skip whitespace
  int c;

  while (isspace(c = fgetc(in)))
    ;

  if (c == EOF) {
    tok.type = TOK_EOF;
    return tok;
  }

  if (c == '(') {
    tok.type = TOK_LPAREN;
    return tok;
  }

  if (c == ')') {
    tok.type = TOK_RPAREN;
    return tok;
  }

  if (c == '\'') {
    tok.type = TOK_QUOTE;
    return tok;
  }

  if (c == ';') {
    tok.type = TOK_COMMENT;

    // Skip comment
    for (; c != EOF && c != '\n'; c = fgetc(in))
      ;

    return tok;
  }

  if (c == '#') {
    tok.type = TOK_BOOL;
    buf[0] = '#';
    buf[1] = fgetc(in);
    buf[2] = '\0';

    if (!strchr("tf", buf[1]))
      repl_error("Malformated input: %s\n", buf);

    // tok.text = strdup(buf);

    return tok;
  }

  // Number
  if (isdigit(c) || (c == '-' && isdigit(peek(in)))) {
    int i = 0;

    buf[i++] = c;
    while (isdigit(peek(in)) || strchr("./", peek(in)))
      buf[i++] = fgetc(in);
    buf[i] = '\0';

    tok.type = TOK_NUMBER;
    return tok;
  }

  // Symbol
  if (isalpha(c) || strchr("+-*/<=>!?_", c)) {
    int i = 1;

    buf[0] = c;

    while (isalnum(peek(in)) || strchr("+-*/<=>!?_", peek(in)))
      buf[i++] = fgetc(in);

    buf[i] = '\0';

    tok.type = TOK_SYMBOL;
    return tok;
  }

  // String literal
  if (c == '\"') {
    int i = 0;

    // TODO: Add escapes
    c = fgetc(in);
    while (isprint(c) && '\"' != c) {
      buf[i] = c;
      c = fgetc(in);
      i += 1;
    }

    buf[i] = '\0';

    if (c != '\"')
      repl_error("Unterminated string literal: %s", buf);

    tok.type = TOK_STRING;
    return tok;
  }

  repl_error("Unexpected char: '%c'\n", c);
}

/**
 * Simple parser
 */

// Greey parse all sexps in input
value parse_all(FILE *in, env e) {
  value exprs = value_new_nil();
  value *tail = &exprs;

  for (;;) {
    value expr = parse_expression(in, e);

    if (bool_isnil(expr, e))
      return exprs;

    *tail = value_new_cons(expr, value_new_nil());
    tail = &(*tail)->cons.cdr;
  }
}

value parse_list(FILE *in, env e) {
  token t = token_getnext(in);

  if (t.type == TOK_EOF)
    repl_error("Unbalanced parenthesis");

  if (t.type == TOK_RPAREN)
    return value_new_nil();

  token_push(t);

  value car_val = parse_expression(in, e);
  value cdr_val = parse_list(in, e);

  return value_new_cons(car_val, cdr_val);
}

value parse_expression(FILE *in, env e) {

  // Loop needed for comment, aka skip
  for (;;) {
    token t = token_getnext(in);

    switch (t.type) {
    case TOK_LPAREN:
      return parse_list(in, e);

    case TOK_QUOTE:
      return value_new_cons(
          value_new_symbol("quote", e),
          value_new_cons(parse_expression(in, e), value_new_nil()));

    case TOK_BOOL:
      return value_new_bool(t.text[1] == 't');

    case TOK_NUMBER: {
      mpq_ptr exact = num_exact_new();

      if (0 != mpq_set_str(exact, t.text, 10))
        repl_error("Invalid number string: %s", t.text);

      mpq_canonicalize(exact);

      return value_new_exact(exact);
    }

    case TOK_SYMBOL:
      return value_new_symbol(t.text, e);

    case TOK_STRING:
      return value_new_string(strdup(t.text));

    case TOK_RPAREN:
      repl_error("Unexpected ')' outside list");

    case TOK_EOF:
      return value_new_nil();

    case TOK_COMMENT:
      continue;

    default:
      repl_error("Unknown token type");
    }
  }
}
