#include "mpc.h"
#include "functions.h"

static char *VERSION_STRING = "0.12.0";

// Lisp Value
struct lval {
    int type;

    long num;
    char* err;
    char* sym;
    lbuiltin fun;

    int count;
    lval** cell;
};

// Lisp Environment
struct lenv {
  int count;
  char **syms;
  lval **vals;
};

/* LENV HELPER FUNCTIONS
 * lenv_new
 * lenv_del
 * lenv_get
 * lenv_put
 */

lenv *lenv_new(void) {
  lenv *e = malloc(sizeof(lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void lenv_del(lenv *e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}

lval* lenv_get(lenv* e, lval* k) {
  // Iterate over all items in environment
  for (int i = 0; i < e->count; i++) {
    // Check if the stored string matches the symbol string
    // If it does, return a copy of the value
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }
  // If no symbol fonud return an error
  return lval_err("Unbound symbol '%s'", k->sym);
}

void lenv_put(lenv *e, lval *k, lval *v) {

  // Iterate over all items in environment
  // to see if variable already exists
  for (int i = 0; i < e->count; i++) {

    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }

  // If no existing entry found, allocate space for new entry
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);

  // Copy contents of lval and symbol string into new location
  e->vals[e->count-1] = lval_copy(v);
  e->syms[e->count-1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1], k->sym);
}

void lenv_print(lenv *e) {
  for (int i = 0; i < e->count; i++) {
    printf("%s\n", e->syms[i]);
  }
}

void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
  lval *k = lval_sym(name);
  lval *v = lval_fun(func);

  lenv_put(e, k, v);
  lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv *e) {
  // List Functions
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "len",  builtin_len);
  lenv_add_builtin(e, "cons", builtin_cons);
  lenv_add_builtin(e, "init", builtin_init);

  // Mathematical Functions
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);

  // Variable Functions
  lenv_add_builtin(e, "def", builtin_def);

  // Environment functions
  lenv_add_builtin(e, "env", builtin_env);
  lenv_add_builtin(e, "exit", builtin_exit);
}





/* LVAL CONSTRUCTOR FUNCTIONS
 *
 * lval_fun
 * lval_num
 * lval_err
 * lval_sym
 * lval_sexpr
 * lval_qexpr
 */

// Construct a pointer to a new Function lval
lval *lval_fun(lbuiltin func) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->fun = func;
  return v;
}
// Construct a pointer to a new Number lval
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// Construct a pointer to a new Error lval
lval* lval_err(char* fmt, ...) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    // Create a va_list and initialize it
    va_list va;
    va_start(va, fmt);

    v->err = malloc(512);

    // printf the error string with a maximum of 511 characters
    vsnprintf(v->err, 511, fmt, va);

    // reallocate to number of bites actually used
    v->err = realloc(v->err, strlen(v->err)+1);

    // clean up our va list
    va_end(va);

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

// A pointer to a new empty Qexpr lval
lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}




/* LVAL HELPER FUNCTIONS
 *
 * lval_copy
 * lval_del
 * lval_read_num
 * lval_read
 * lval_add
 * lval_expr_print
 * lval_print
 * lval_println
 * lval_join
 * lval_pop
 * lval_take
 * lval_eval_sexpr
 * lval_eval
*/

lval *lval_copy(lval *v) {

    lval *x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
      // Copy Functions and Numbers Directly
      case LVAL_FUN: x->fun = v->fun; break;
      case LVAL_NUM: x->num = v->num; break;

      // Copy Strings using malloc and strcpy
      case LVAL_ERR:
        x->err = malloc(strlen(v->err) + 1);
        strcpy(x->err, v->err); break;

      case LVAL_SYM:
        x->sym = malloc(strlen(v->sym) + 1);
        strcpy(x->sym, v->sym); break;

      // Copy lists by copying each sub-expression
      case LVAL_SEXPR:
      case LVAL_QEXPR:
        x->count = v->count;
        x->cell =  malloc(sizeof(lval*) * x->count);
        for (int i = 0; i < x->count; i++) {
          x->cell[i] = lval_copy(v->cell[i]);
        }
      break;
    }

    return x;
}

void lval_del(lval* v) {
  switch (v->type) {
    // Do nothing special for number type
    case LVAL_FUN: break;
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
        case LVAL_FUN: printf("<function>"); break;
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

lval* lval_join(lval* x , lval* y) {
  // For each cell in 'y' add it to 'x'
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  // Delete the empty 'y' and return 'x'
  lval_del(y);
  return x;
}

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

lval* lval_eval_sexpr(lenv *e, lval *v) {

  // Evaluate children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
  }

  // Error Checking
  for (int i = 0; i  < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  // Empty Expression
  if (v->count == 0) { return v; }

  // Single Expression
  if (v->count==1) { return lval_take(v, 0); }

  // Ensure First Element is a function after evaluation
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_FUN) {
    lval_del(v); lval_del(f);
    return lval_err("first element is not a function");
  }

  // If so call function to get result
  lval* result = f->fun(e, v);
  lval_del(f);
  return result;
}

lval *lval_eval(lenv *e, lval *v) {
  if (v->type == LVAL_SYM) {
    lval *x = lenv_get(e, v);
    lval_del(v);
    return x;
  }

  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
  return v;
}

lval *builtin_def(lenv *e, lval *a) {
  LASSERT_TYPE(a, "def", 0, LVAL_QEXPR);

  // TODO: Ensure a builtin variable does not exist with this name
  LASSERT(a, a,
    "Cannot redefine a builtin variable");

  // first argument is a symbol list
  lval *syms = a->cell[0];

  // ensure all elements of first list are symbols
  for (int i = 0; i < syms->count; i++) {
    LASSERT_TYPE(a, "def", i, LVAL_SYM);
  }

  // check correct number of symbols and values
  LASSERT(a, syms->count == a->count-1,
    "Function 'def' cannot define incorrect "
    "number of values to symbols. Got %i keys and %i values",
    syms->count, a->count-1);

  // assign copies of values to symbols
  for (int i = 0; i < syms->count; i++) {
    lenv_put(e, syms->cell[i], a->cell[i+1]);
  }

  lval_del(a);
  return lval_sexpr();
}




/*
 *  Rosq Built In Functions
 */

//  builtin_len() returns the number of elements in a Q-Expression
lval* builtin_len(lenv *e, lval *a) {
   LASSERT_TYPE(a, "len", 0, LVAL_QEXPR);
   LASSERT_NUM(a, "len", 1);

   lval* count = lval_num(a->cell[0]->count);

   return count;
}

//  builtin_head() returns just the first element, deletes rest
lval* builtin_head(lenv *e, lval *a) {
  LASSERT_NUM(a, "head", 1);
  LASSERT_NOT_EMPTY(a);
  LASSERT_TYPE(a, "head", 0, LVAL_QEXPR);

  // Otherwise take first argument
  lval* v = lval_take(a, 0);

  // Delete all elements that are not head and return
  while (v->count > 1) { lval_del(lval_pop(v, 1)); }
  return v;
}

//  builtin_tail() deletes first element, returns rest
lval* builtin_tail(lenv *e, lval *a) {
  // Check error conditions
  LASSERT_NUM(a, "tail", 1);
  LASSERT_TYPE(a, "tail", 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY(a);

  // Take first argument
  lval* v = lval_take(a, 0);

  // Delete first element and return
  lval_del(lval_pop(v, 0));
  return v;
}

//  builtin_init() returns all of a Q-Expression except the final element
lval* builtin_init(lenv *e, lval *a) {
  LASSERT_NUM(a, "init",  1);
  LASSERT_TYPE(a, "init", 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY(a);

  lval* v = lval_take(a, 0);

  lval_pop(v, v->count - 1);
  return v;
}

//  builtin_list() turns things into a list S-Expression
lval* builtin_list(lenv *e, lval *a) {
  a->type = LVAL_QEXPR;
  return a;
}

//  builtin_join() joins 2+ Q-Expressions
lval* builtin_join(lenv *e, lval *a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT_TYPE(a, "join", i, LVAL_QEXPR);
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

//  builtin_cons() takes a value and a Q-Expression and appends the value to the front of the Q-Expression
lval* builtin_cons(lenv *e, lval *a) {
  LASSERT_NUM(a, "cons", 2);
  LASSERT_TYPE(a, "cons", 0, LVAL_NUM);
  LASSERT_TYPE(a, "cons", 1, LVAL_QEXPR);
  // New Q-Expression
  lval* newQ = lval_qexpr();
  newQ = lval_add(newQ, lval_pop(a, 0));
  lval* oldQ = lval_take(a, 0);
  newQ = lval_join(newQ, oldQ);

  return newQ;
}

//  builtin_eval() evaluates contents of a Q-Expression
lval* builtin_eval(lenv *e, lval *a) {
  LASSERT_NUM(a, "eval", 1);
  LASSERT_TYPE(a, "eval", 0, LVAL_QEXPR);
  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

//  builtin_op() evaluates arithmetical operations
lval* builtin_op(lenv *e, lval *a, char* op) {
  // Ensure all arguments are numbers
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on a non-number! "
        "Got a %s", ltype_name(a->cell[i]->type));
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

lval *builtin_add(lenv *e, lval *a) {
  return builtin_op(e, a, "+");
}
lval *builtin_sub(lenv *e, lval *a) {
  return builtin_op(e, a, "-");
}
lval *builtin_mul(lenv *e, lval *a) {
  return builtin_op(e, a, "*");
}
lval *builtin_div(lenv *e, lval *a) {
  return builtin_op(e, a, "/");
}

// TODO: env requires an argument at the moment; should not.
lval *builtin_env(lenv *e, lval *a) {
  lenv_print(e);
  return a;
}

lval *builtin_exit() {
  exit(0);
}


/*
 *  MAIN
 */

int main(int argc, char** argv) {
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Symbol   = mpc_new("symbol");
    mpc_parser_t* Sexpr    = mpc_new("sexpr");
    mpc_parser_t* Qexpr    = mpc_new("qexpr");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Rosq     = mpc_new("rosq");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                       \
            number   : /-?[0-9]+/ ;                             \
            symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/;        \
            sexpr    : '(' <expr>* ')' ;                        \
            qexpr    : '{' <expr>* '}' ;                        \
            expr     : <number> | <symbol> | <sexpr> | <qexpr> ;\
            rosq     : /^/ <expr>* /$/ ;              \
        ",
        Number, Symbol, Sexpr, Qexpr, Expr, Rosq);

    printf("Rosq Version %s\n", VERSION_STRING);
    puts("Press Ctrl+C to Exit, or type 'exit 1'\n");

    lenv *e = lenv_new();
    lenv_add_builtins(e);

    while (1) {
        char* input = readline("Rosq> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Rosq, &r)) {
            lval* x = lval_eval(e, lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    lenv_del(e);
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Rosq);

    return 0;
}
