#include "mpc.h"
#include <stdbool.h>

#ifdef _WIN32

  static char buffer[2048];

  char *readline(char *prompt) {
      fputs(prompt, stdout);
      fgets(buffer, 2048, stdin);
      char *cpy = malloc(strlen(buffer)+1);
      strcopy(cpy, buffer);
      cpy[strlen(cpy)-1] = '\0';
      return cpy;
  }

  void add_history(char *unused) {}

#else
  #include <editline/readline.h>
#endif

// Macros
#define LASSERT(args, cond, fmt, ...) \
if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(args, func, index, expect) \
    LASSERT(args, args->cell[index]->type == expect, \
        "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
        func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(args, func, num) \
    LASSERT(args, args->count == num, \
        "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
        func, args->count, num)

#define LASSERT_NOT_EMPTY(args, func, index) \
    LASSERT(args, args->cell[index]->count != 0, \
        "Function '%s' passed {} for argument %i.", func, index);


// Forward declare functions


enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_BOOL, LVAL_STR,
       LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

char *ltype_name(int t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_STR: return "String";
    case LVAL_NUM: return "Number";
    case LVAL_BOOL: return "Boolean";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

typedef lval*(*lbuiltin)(lenv*, lval*);

mpc_parser_t *String ;
mpc_parser_t *Comment;
mpc_parser_t *Number ;
mpc_parser_t *Symbol ;
mpc_parser_t *Sexpr  ;
mpc_parser_t *Qexpr  ;
mpc_parser_t *Expr   ;
mpc_parser_t *Rosq   ;

lval *lval_read_num(mpc_ast_t *t);
lval *lval_read(mpc_ast_t *t);

lenv *lenv_new(void);
void lenv_del(lenv *e);
lval *lenv_get(lenv *e, lval *k);
void lenv_put(lenv *e, lval *k, lval *v);
void lenv_add_builtin(lenv *e, char *name, lbuiltin func);
void lenv_add_builtins(lenv *e);
void lenv_print(lenv *e);

lval *lval_fun(lbuiltin func);
lval *lval_num(long x);
lval *lval_err(char *fmt, ...);
lval *lval_sym(char *s);
lval *lval_sexpr(void);
lval *lval_qexpr(void);

lval *lval_copy(lval *v);
void lval_del(lval *v);
lval *lval_add(lval *v, lval *x);
void lval_expr_print(lval *v, char open, char close);
void lval_print(lval *v);
void lval_println(lval *v);
void lval_print_str(lval *v);
int lval_eq(lval *x, lval *y);
lval *lval_join(lval *x , lval *y);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *lval_eval_sexpr(lenv *e, lval *v);
lval *lval_eval(lenv *e, lval *v);
lval *lval_call(lenv *e, lval *f, lval *a);

lval *builtin_load(lenv *e, lval *a);
lval *builtin_print(lenv *e, lval *a);
lval *builtin_error(lenv *e, lval *a);

lval *builtin_if(lenv *e, lval *a);
lval *builtin_and(lenv *e, lval *a);
lval *builtin_or(lenv *e, lval *a);
lval *builtin_not(lenv *e, lval *a);

lval *builtin_cmp(lenv *e, lval *a, char *op);
lval *builtin_eq(lenv *e, lval *a);
lval *builtin_ne(lenv *e, lval *a);

lval *builtin_ord (lenv *e, lval *a, char *op);
lval *builtin_lt(lenv *e, lval *a);
lval *builtin_lte(lenv *e, lval *a);
lval *builtin_gt(lenv *e, lval *a);
lval *builtin_gte(lenv *e, lval *a);

lval *builtin_lamba(lenv *e, lval *a);
lval *builtin_put(lenv *e, lval *a);
lval *builtin_var(lenv *e, lval *a, char *func);
lval *builtin_def(lenv *e, lval *a);
lval *builtin_len(lenv *e, lval *a);
lval *builtin_head(lenv *e, lval *a);
lval *builtin_tail(lenv *e, lval *a);
lval *builtin_init(lenv *e, lval *a);
lval *builtin_list(lenv *e, lval *a);
lval *builtin_join(lenv *e, lval *a);
lval *builtin_cons(lenv *e, lval *a);
lval *builtin_eval(lenv *e, lval *a);
lval *builtin_op(lenv *e, lval *a, char *op);
lval *builtin_add(lenv *e, lval *a);
lval *builtin_sub(lenv *e, lval *a);
lval *builtin_mul(lenv *e, lval *a);
lval *builtin_div(lenv *e, lval *a);
lval *builtin_env(lenv *e, lval *a);
lval *builtin_exit();
