#include "mpc.h"
#include <math.h>

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

#define LASSERT(args, cond, err) if (!(cond)) { lval_del(args); return lval_err(err); }

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_FUN, LVAL_SEXPR , LVAL_QEXPR};

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
    int type;
    long num;
    char* err;
    char* sym;
    lbuiltin fun;
    int count;
    struct lval** cell;
};

struct lenv {
    int count;
    char** syms;
    lval** vals;
}

void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);
void lval_del(lval* v);
lval* lval_num(long x);
lval* lval_err(char* m);
lval* lval_sym(char* s);
lval* lval_sexpr(void);
lval* lval_qexpr(void);
lval* lval_add(lval* v, lval* x);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
lval* lval_eval(lval* v);
lval* lval_eval_sexpr(lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_join(lval* x, lval* y);
lval* builtin(lval* a, char* func);
lval* builtin_op(lval* a, char* op);
lval* builtin_head(lval* a);
lval* builtin_tail(lval* a);
lval* builtin_list(lval* a);
lval* builtin_eval(lval* a);
lval* builtin_join(lval* a);
lenv* lenv_new(void);
lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* k, lval* v){
void lenv_del(lenv* e);

