#ifndef REPL_H
#define REPL_H

#include <setjmp.h>
#include <stdio.h>

#include "env.h"
#include "value.h"

extern jmp_buf repl_env;

// Return to repl prompt on error
__attribute__((noreturn)) void repl_error(const char *fmt, ...);

// Main repl
void repl(env e);

// eval and print last result
value repl_eval(FILE *in, env e);

// for loading
value repl_eval_file(char *filename, env e);

#endif
