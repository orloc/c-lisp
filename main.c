#include "mpc.h"
#include <math.h>


typedef struct {
    int type;
    long num;
    int err;
} lval;

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

double eval(mpc_ast_t* t);
double eval_op(double x, char* op, double y);
int count_leaves(mpc_ast_t* t);
int count_branches(mpc_ast_t* t);
double min (double x, double y);
double max (double x, double y);


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

int main(int argc, char** argv){

    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                       \
            number      : /-?[0-9]+\\.?[0-9]*/;                                 \
            operator    : '+' | '-' | '/' | '*' | '%' | '^' | \"min\" | \"max\";\
            expr        : <number> | '(' <operator> <expr>+ ')';                \
            lispy       : /^/ <operator> <expr>+ /$/;                           \
        ",
    Number, Operator, Expr, Lispy);

    puts("Lispy Version 0.0.0.1");
    puts("Ctrl+c to Exit\n");

    while(1){
        char* input = readline("lispy> ");
        add_history(input);

        mpc_result_t r;

        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            mpc_ast_print(r.output);
            puts("\n");
            double result = eval(r.output);
            printf("Result: %f\n", result);
            printf("Leaf Count: %i\n", count_leaves(r.output));
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;
}

double eval(mpc_ast_t* t){

    if (strstr(t->tag, "number") != 0) { return atof(t->contents); }

    char* op = t->children[1]->contents;
    double x = eval(t->children[2]);

    int i = 3;
    while(i < t->children_num && strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

double eval_op(double x, char* op, double y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "%") == 0) { return fmod(x,y); }
    if (strcmp(op, "^") == 0) { return pow(x,y); }
    if (strcmp(op, "min") == 0) { return min(x,y); }
    if (strcmp(op, "max") == 0) { return max(x,y); }

    return 0;
}

int count_leaves(mpc_ast_t* t){
    if (t->children_num == 0){
        return 1;
    }

    int i = 0;
    int count = 0;
    while( i < t->children_num ){
        count += count_leaves(t->children[i]);
        i++;
    }

    return count;
}

int count_branches(mpc_ast_t* t){
    return 1;
}

double min (double x, double y){
    if (x > y) { return y; }
    return x;
}

double max (double x, double y){
    if (x < y) { return y; }
    return x;
}

void lval_print(lval v){
    switch (v.type) {
        case LVAL_NUM: printf("%li", v.num);break;
        case LVAL_ERR:
            if (v.err == LERR_DIV_ZERO) { printf("Error: Division by Zero!"); }
            if (v.err == LERR_BAD_OP) { printf("Error: Division by Zero!"); }
            if (v.err == LERR_BAD_NUM) { printf("Error: Division by Zero!"); }
            break;
    }
}
 void lval_println(lval v) { lval_print(v); putchar('\n');}

lval lval_num(long x){
    lval v;
    v.type = LVAL_ERR;
    v.num = x;
    return v;
}

lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}
