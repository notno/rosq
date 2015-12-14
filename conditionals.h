
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
  if (!(cond)) { \
    lval *err = lval_err(fmt, ##__VA_ARGS__); \
    lval_del(args); \
    return err; \
  }
#define LASSERT_TYPE(args, name, index, type_needed) \
  if (a->cell[index]->type != type_needed) { \
    "Function %s passed incorrect type for argument %i. Got %s. Wanted %s", \
    name, index, ltype_name(args->cell[index]->type), ltype_name(type_needed); \
  }
#define LASSERT_NUM(args, func_name, num_needed) \
  if (args->count != num_needed) { \
    lval *err = lval_err("Function %s got wrong # of arguments, %i needed, %i given", \
    func_name, num_needed, args->count); \
    lval_del(args); \
    return err; \
  }
#define LASSERT_NOT_EMPTY(args) \
  if (args->count == 0) { lval_del(args); return lval_err("Arguments missing."); }

// Forward declare functions

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM,
       LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

char *ltype_name(int t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}

typedef lval*(*lbuiltin)(lenv*, lval*);

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
lval *lval_read_num( mpc_ast_t *t);
lval *lval_read(mpc_ast_t *t);
lval *lval_add(lval *v, lval *x);
void lval_expr_print(lval *v, char open, char close);
void lval_print(lval *v);
void lval_println(lval *v);
int lval_compare(lval *a, lval *b);
lval *lval_join(lval *x , lval *y);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *lval_eval_sexpr(lenv *e, lval *v);
lval *lval_eval(lenv *e, lval *v);
lval *lval_call(lenv *e, lval *f, lval *a);

lval *builtin_comparison (lenv *e, lval *a, char *op);
lval *builtin_lt(lenv *e, lval *a);
lval *builtin_lte(lenv *e, lval *a);
lval *builtin_gt(lenv *e, lval *a);
lval *builtin_gte(lenv *e, lval *a);
lval *builtin_equals(lenv *e, lval *a);
lval *builtin_not_equals(lenv *e, lval *a);
lval *builtin_if(lenv *e, lval *a);

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
