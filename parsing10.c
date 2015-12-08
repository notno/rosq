#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"

#ifdef _WIN32
  #include <string.h>

  static char buffer[2048];

  char* readline(char* prompt) {
      fputs(prompt, stdout);
      fgets(buffer, 2048, stdin);
      char* cpy = malloc(strlen(buffer)+1);
      strcopy(cpy, buffer);
      cpy[strlen(cpy)-1] = '\0';
      return cpy;
  }

  void add_history(char* unused) {}

  #else
  #include <editline/readline.h>
#endif

#define LASSERT(args, cond, err) \
  if (!(cond)) { lval_del(args); return lval_err(err); }

typedef struct lval {
    int type;
    long num;
    // Error and Symbol types have some string data
    char* err;
    char* sym;
    // Count and Pointer to a list of lval*
    int count;
    struct lval** cell;
} lval;

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

// Forward declare functions
lval* lval_add(lval* v, lval* x);
lval* lval_eval(lval* v);
void lval_print(lval* v);

// Construct a pointer to a new Number lval
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// Construct a pointer to a new Error lval
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

// Construct a pointer to a new Symbol lval
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

// A pointer to a new empty Sexpr lval
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval* v) {
  switch (v->type) {
    // Do nothing special for number type
    case LVAL_NUM: break;

    // For Err or Sym froo the string data
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

    // If Qexpr or Sexpr then deleet all elements inside
    case LVAL_QEXPR:
    case LVAL_SEXPR:
      for( int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      // Also free the memory allocated to contain the pointers
      free(v->cell);
  }

  // Free the memory allocated for the 'lval' struct itself
  free(v);
}

lval* lval_read_num( mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ?
    lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
  // If Symbol or Number return conversion to that type
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  // if root (>) or sexpr then create empty list
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

  // Fill this list with any valid expression contained within
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {

    // Print Value contained within
    lval_print(v->cell[i]);

    // Don't print trailing spaces if last element
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

/* print an 'lval' */
void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_NUM: printf("%li", v->num); break;
        case LVAL_ERR: printf("Error: %s", v->err); break;
        case LVAL_SYM: printf("%s", v->sym); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
        case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
        break;
    }
}

/* print an 'lval' followed by a newline */
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_pop(lval* v, int i) {
  // find the item at i
  lval* x = v->cell[i];

  // shift memory after the item at i over the top
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

  // decrease the count of items in the list
  v->count--;

  //reallocate the memory used
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* builtin_head(lval* a) {
  LASSERT(a, a->count == 1,
    "Function 'head' passed incorrect type!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'head' passed incerroct type!");
  LASSERT(a, a->cell[0]->count != 0,
    "Function 'head' passed {}!");

  // Otherwise take first argument
  lval* v = lval_take(a, 0);

  // Delete all elements that are not head and return
  while (v->count > 1) { lval_del(lval_pop(v, 1)); }
  return v;
}

lval* builtin_tail(lval* a) {
  // Check error conditions
  LASSERT(a, a->count ==1,
    "Function 'tail' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'tail' passed incorrect type!");
  LASSERT(a, a->cell[0]->count != 0,
    "Function 'tail' passed {}!");

  // Take first argument
  lval* v = lval_take(a, 0);

  // Delete first element and return
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_op(lval* a, char* op) {
  // Ensure all arguments are numbers
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on a non-number!");
    }
  }

  // Pop the first element
  lval* x = lval_pop(a, 0);

  // If no arguments and sub then perform unary negation
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  // While there are still elements remaining
  while (a->count > 0) {

    // Pop the next element
    lval* y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "%") == 0) { x->num %= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division By Zero!"); break;
      }
      x->num /= y->num;
    }

    lval_del(y);
  }

  lval_del(a); return x;
}

lval* lval_eval_sexpr(lval* v) {

  // Evaluate children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  // Error Checking
  for (int i = 0; i  < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  // Empty Expression
  if (v->count == 0) { return v; }

  // Single Expression
  if (v->count==1) { return lval_take(v, 0); }

  // Ensure First Element is Symbol
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S-expression does not start with symbol!");
  }

  // Call builtin with operator
  lval* result = builtin_op(v, f->sym);
  lval_del(f);
  return result;
}

lval* lval_eval(lval* v) {
  // Evaluate Sexpressions
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
  // All other lval types remain the same
  return v;
}

int main(int argc, char** argv) {
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Symbol   = mpc_new("symbol");
    mpc_parser_t* Sexpr    = mpc_new("sexpr");
    mpc_parser_t* Qexpr    = mpc_new("qexpr");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Rosq     = mpc_new("rosq");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                               \
            number   : /-?[0-9]+/ ;                     \
            symbol   : \"list\" | \"head\" | \"tail\"   \
                     | \"join\" | \"eval\" | '+' | '-'  \
                     | '*' | '/' | '%' ;                \
            sexpr    : '(' <expr>* ')' ;                \
            qexpr    : '{' <expr>* '}' ;                \
            expr     : <number> | <symbol> | <sexpr> | <qexpr> ;\
            rosq     : /^/ <expr>* /$/ ;              \
        ",
        Number, Symbol, Sexpr, Qexpr, Expr, Rosq);

    puts("Rosq Version 0.0.0.10.2");
    puts("Press Ctrl+C to Exit\n");

    while (1) {
        char* input = readline("Rosq> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Rosq, &r)) {
            lval* x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Rosq);

    return 0;
}
