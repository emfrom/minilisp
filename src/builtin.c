#include <assert.h>
#include <gmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "builtin.h"
#include "env.h"
#include "numbers.h"
#include "repl.h"
#include "value.h"

/**
 *  Builtin functions
 */

value builtin_add(value args, env e) {
  mpq_ptr sum = num_exact_new();

  // Additive identity
  mpq_set_ui(sum, 0, 1);

  while (args->type == TYPE_CONS) {
    value v = car(args);

    if (v->type != TYPE_NUM_EXACT)
      repl_error("add: expects number");

    mpq_add(sum, sum, v->num_exact);

    args = cdr(args);
  }

  return value_new_exact(sum);
}

value builtin_sub(value args, env e) {
  value first = car(args);
  value rest = cdr(args);

  mpq_ptr result = num_exact_new();

  if (first->type != TYPE_NUM_EXACT)
    repl_error("sub: expects number");

  // unary: negate
  if (rest->type == TYPE_NIL) {
    mpq_neg(result, first->num_exact);
    return value_new_exact(result);
  }

  mpq_set(result, first->num_exact);

  for (; rest->type != TYPE_NIL; rest = cdr(rest)) {
    value v = car(rest);

    if (v->type != TYPE_NUM_EXACT)
      repl_error("sub: expects number");

    mpq_sub(result, result, v->num_exact);
  }

  return value_new_exact(result);
}

value builtin_mult(value args, env e) {
  mpq_ptr product = num_exact_new();

  // Multiplicative identity
  mpq_set_ui(product, 1, 1);

  while (args->type == TYPE_CONS) {
    value v = car(args);

    if (v->type != TYPE_NUM_EXACT)
      repl_error("add: expects number");

    mpq_mul(product, product, v->num_exact);

    args = cdr(args);
  }

  return value_new_exact(product);
}

value builtin_div(value args, env e) {
  value first = car(args);
  value rest = cdr(args);

  mpq_ptr result = num_exact_new();

  if (first->type != TYPE_NUM_EXACT)
    repl_error("div: expects number");

  // unary: negate
  if (rest->type == TYPE_NIL) {
    mpq_inv(result, first->num_exact);
    return value_new_exact(result);
  }

  mpq_set(result, first->num_exact);

  for (; rest->type != TYPE_NIL; rest = cdr(rest)) {
    value v = car(rest);

    if (v->type != TYPE_NUM_EXACT)
      repl_error("div: expects number");

    mpq_div(result, result, v->num_exact);
  }

  return value_new_exact(result);
}

int bool_isnil(value args, env e) {
  if (args->type == TYPE_NIL)
    return 1;

  return 0;
}

value builtin_null_pred(value args, env e) {
  if (bool_isnil(args, e))
    repl_error("null? takes exactly one argument");

  return value_new_bool(bool_isnil(car(args), e));
}

int bool_istrue(value args, env e) {
  if (args->type == TYPE_BOOL)
    return args->boolean;

  return 1; // everything else true
}

value builtin_true_pred(value args, env e) {
  if (args->type != TYPE_CONS || cdr(args)->boolean != TYPE_NIL)
    repl_error("true? needs one argument");

  return value_new_bool(bool_istrue(car(args), e));
}

int bool_isnumber(value args, env e) {
  if (args->type == TYPE_NUM_EXACT)
    return 1;

  // if(args->type == TYPE_NUM_INEXACT)
  //   return 1;

  return 0;
}

value builtin_isnumber(value args, env e) {
  assert(args->type == TYPE_CONS);

  return value_new_bool(bool_isnumber(car(args), e));
}

value builtin_equ(value args, env e) {
  if (args->type == TYPE_NIL) {
    // Composable logic dictates
    return value_new_bool(1);
  }

  value prev = car(args);

  if (!bool_isnumber(prev, e)) {
    repl_error("=: arguments must be numbers");
  }

  args = cdr(args);

  while (args->type != TYPE_NIL) {
    value curr = car(args);

    if (!bool_isnumber(curr, e))
      repl_error("=: arguments must be numbers");

    // Compare prev == curr
    if (!mpq_equal(prev->num_exact, curr->num_exact))
      return value_new_bool(0);

    prev = curr;
    args = cdr(args);
  }

  return value_new_bool(1);
}

value builtin_lequ(value args, env e) {
  if (args->type == TYPE_NIL) {
    // Composable logic dictates
    return value_new_bool(1);
  }

  value prev = car(args);

  if (!bool_isnumber(prev, e)) {
    repl_error("<=: arguments must be numbers");
  }

  args = cdr(args);

  while (args->type != TYPE_NIL) {
    value curr = car(args);

    if (!bool_isnumber(curr, e))
      repl_error("<=: arguments must be numbers");

    // Compare prev <= curr <> ! prev > curr
    if (mpq_cmp(prev->num_exact, curr->num_exact) > 0)
      return value_new_bool(0);

    prev = curr;
    args = cdr(args);
  }

  return value_new_bool(1);
}

value builtin_load(value args, env e) {
  if (args->type != TYPE_CONS)
    repl_error("load need at least one argument");

  do {
    value file = car(args);

    if (file->type != TYPE_STRING)
      repl_error("load takes strings as arguments");

    repl_eval_file(file->string, e);

    args = cdr(args);
  } while (args->type == TYPE_CONS);

  return value_new_bool(1);
}

value builtin_cons(value args, env e) {
  if (args->type != TYPE_CONS || cdr(args)->type != TYPE_CONS)
    repl_error("cons takes two arguments");

  value first = car(args);
  value second = cadr(args);

  // Allocate new cons cell
  value new_cons = value_alloc(TYPE_CONS);

  car(new_cons) = first;
  cdr(new_cons) = second;

  return new_cons;
}

value builtin_car(value args, env e) {
  if (args->type != TYPE_CONS || car(args)->type != TYPE_CONS)
    repl_error("Argument to car not a pair");

  return caar(args);
}

value builtin_cdr(value args, env e) {
  if (args->type != TYPE_CONS || car(args)->type != TYPE_CONS)
    repl_error("Argument to cdr not a pair");

  return cdar(args);
}

// Fist lisp voodo :)
value builtin_list(value args, env e) { return args; }

value builtin_display(value args, env e) {
  if (args->type != TYPE_CONS)
    repl_error("display need at least one argument");

  do {
    value text = car(args);

    if (text->type != TYPE_STRING)
      repl_error("display takes strings as arguments");

    printf("%s", text->string);

    args = cdr(args);
  } while (args->type == TYPE_CONS);

  return value_new_bool(1);
}

value builtin_newline(value args, env e) {
  printf("\n");

  return value_new_bool(1);
}

value builtin_debug(value args, env e) {
  printf("\n0x%p\n", args);

  if (args->type == TYPE_CONS)
    printf("0x%p\n", car(args));

  value_print(args);
  printf("\n\n");

  return value_new_bool(1);
}

value builtin_debugenv(value args, env e) {
  env_dump(e);

  return value_new_bool(1);
}

value builtin_eq_pred(value args, env e) {
  if (args->type != TYPE_CONS || cdr(args)->type != TYPE_CONS ||
      cddr(args)->type != TYPE_NIL)
    repl_error("eq? takes exactly two arguments");

  return car(args) == cadr(args) ? value_new_bool(1) : value_new_bool(0);
}

value builtin_pair_pred(value args, env e) {
  if (args->type != TYPE_CONS || cdr(args)->type != TYPE_NIL)
    repl_error("pair? takes exactly one argument");

  return car(args)->type == TYPE_CONS ? value_new_bool(1) : value_new_bool(0);
}

char *base_exact_to_string(mpq_t number, int base) {
  // integer?
  if (mpz_cmp_ui(mpq_denref(number), 1) == 0)
    return mpz_get_str(NULL, base, mpq_numref(number));

  // Q
  return mpq_get_str(NULL, base, number);
}

value builtin_number_to_string(value args, env e) {
  if (!bool_isnumber(car(args), e))
    repl_error("number->string: takes 1 or 2 numbers as arguments");

  int base = 10;

  if (cdr(args)->type == TYPE_CONS) {
    if (!bool_isnumber(cadr(args), e) || !bool_isnil(cddr(args), e))
      repl_error("number->string: takes 1 or 2 numbers as arguments");

    mpq_ptr base_q = cadr(args)->num_exact;

    mpq_canonicalize(base_q);
    if (mpz_cmp_ui(mpq_denref(base_q), 1) != 0 || mpq_sgn(base_q) <= 0)
      repl_error(
          "number->string: base argument must be a positive whole number");

    base = mpz_get_si(mpq_numref(base_q));
  }

  char *string;

  string = base_exact_to_string(car(args)->num_exact, base);

  return value_new_string(string);
}

struct builtin_functions {
  char *name;
  function fn;
};

struct builtin_functions startup[] = {
    {"+", builtin_add},
    {"-", builtin_sub},
    {"*", builtin_mult},
    {"/", builtin_div},
    {"true?", builtin_true_pred},
    {"null?", builtin_null_pred},
    {"pair?", builtin_pair_pred},
    {"number?", builtin_isnumber},
    {"number->string", builtin_number_to_string},
    {"eq?", builtin_eq_pred},
    {"<=", builtin_lequ},
    {"=", builtin_equ},
    {"load", builtin_load},
    {"cons", builtin_cons},
    {"car", builtin_car},
    {"cdr", builtin_cdr},
    {"list", builtin_list},
    {"display", builtin_display},
    {"newline", builtin_newline},
    {"debug", builtin_debug},
    {"debugenv", builtin_debugenv},
    {NULL, NULL}};

env builtins_startup(env e) {

  for (int i = 0; startup[i].name != NULL; i++) {

    // To void #include loop's
    value symbol = value_alloc(TYPE_SYMBOL);
    value function = value_alloc(TYPE_FUNCTION);

    symbol->sym = startup[i].name;
    function->fn = startup[i].fn;
    env_set(e, symbol, function);
  }

  return e;
}
