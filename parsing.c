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

typedef struct {
    int type;
    long num;
    int err;
} lval;

enum lval { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };
enum lerr { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

/* print an 'lval' */
void lval_print(lval v) {
    switch (v.type) {
        /* In the case that the type is a number print it */
        /* Then 'break' out of the switch */
        case LVAL_NUM: printf("%li", v.num); break;

        /* In the case the type is an error */
        case LVAL_ERR:
            /* Check what type of error it is and print it */
            if (v.err == LERR_DIV_ZERO) {
                printf("Error: Division by ZERO!");
            }
            if (v.err == LERR_BAD_OP) {
                printf("Error: Invalid Operator");
            }
            if (v.err == LERR_BAD_NUM) {
                printf("Error: Invalid number!");
            }
        break;
    }
}

/* print an 'lval' followed by a newline */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y) {

    /* If either value is an error return it */
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    /* Otherwise do maths on the number values */
    if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if (strcmp(op, "/") == 0) {
        /* If second operand is zero return error */
        return y.num == 0
          ? lval_err(LERR_DIV_ZERO)
          : lval_num(x.num / y.num);
    }
    if (strcmp(op, "%") == 0) {
        return y.num == 0
          ? lval_err(LERR_DIV_ZERO)
          : lval_num(x.num % y.num);
    }

    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {

    if (strstr(t->tag, "number")) {
        /* Check if there is some error in conversion */
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    char* op = t->children[1]->contents;

    lval x = eval(t->children[2]);

    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char** argv) {
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Symbol   = mpc_new("symbol");
    mpc_parser_t* Sexpr    = mpc_new("sexpr");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Rosq     = mpc_new("rosq");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                             \
            number   : /-?[0-9]+/ ;                   \
            symbol   : '+' | '-' | '*' | '/' | '%' ;  \
            sexpr    : '(' <expr>* ')';              \
            expr     : <number> | <symbol> | <sexpr> ;\
            rosq     : /^/ <expr>* /$/ ;              \
        ",
        Number, Symbol, Sexpr, Expr, Rosq);

    puts("Rosq Version 0.0.0.0.1");
    puts("Press Ctrl+C to Exit\n");

    while (1) {
        char* input = readline("Rosq> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Rosq, &r)) {
            lval result = eval(r.output);
            lval_println(result);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(4, Number, Symbol, Sexpr, Expr, Rosq);

    return 0;
}
