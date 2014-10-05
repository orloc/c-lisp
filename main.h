#include "mpc.h"
#include <math.h>


typedef struct lval {
    int type;
    long num;

    char* err;
    char* sym;

    int count;

    struct lval** cell;
} lval;

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

double eval(mpc_ast_t* t);
double eval_op(double x, char* op, double y);
void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);
void lval_del(lval* v);
lval* lval_num(long x);
lval* lval_err(char* m);
lval* lval_sym(char* s);
lval* lval_sexpr(void);
lval* lval_add(lval* v, lval* x);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
}

void add_history(char* unused) {}

#else
    // ubuntu
    #ifdef __READLINE__
        #include <readline.h>
    #else
    // arch
        #include <editline/readline.h>
    #endif
#endif
