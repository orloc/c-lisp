#include "main.h"

int main(int argc, char** argv){

    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Symbol  = mpc_new("symbol");
    mpc_parser_t* Sexpr  = mpc_new("sexpr");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                       \
            number      : /-?[0-9]+/;                                            \
            symbol    : '+' | '-' | '/' | '*';                                  \
            sexpr    : '(' <expr>* ')';                                         \
            expr        : <number> | <symbol> | <sexpr> ;                       \
            lispy       : /^/ <expr>* /$/;                                      \
        ",
    Number, Symbol, Sexpr, Expr, Lispy);

    puts("Lispy Version 0.0.0.2");
    puts("Ctrl+c to Exit\n");

    while(1){
        char* input = readline("lispy> ");
        add_history(input);

        mpc_result_t r;

        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval* x = lval_read(r.output);
            lval_println(x);
            lval_del(x);
        }
        free(input);
    }

    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

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

    return 0;
}


void lval_expr_print(lval* v, char open, char close){
    putchar(open);

    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);
        // dont print trailing space
        if (i != (v->count-1)){
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print(lval* v){
    switch (v->type) {
        case LVAL_NUM: printf("%li", v->num); break;
        case LVAL_ERR: printf("Error: %s", v->err); break;
        case LVAL_SYM: printf("%s", v->sym); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    }
}
 void lval_println(lval* v) {
     lval_print(v);
     putchar('\n');
 }

void lval_del(lval* v){
    switch(v->type){
        // dont do anything
        case LVAL_NUM:  break;
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;

        // We need to recurse into this to delete everything
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++){
                lval_del(v->cell[i]);
            }
            free(v->cell);
           break;
    }

    // free the initial value
    free(v);
}

/* Construct new pointer to a new number lval */
lval* lval_num(long x){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

/* Construct new pointer to a new err lval */
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m)+1);
    strcpy(v->err, m);
    return v;
}

/* Construct new pointer to a new sym lval */
lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s)+1);
    strcpy(v->sym, s);
    return v;
}

/* Construct new pointer to a new err lval */
lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
    // If there is a symbol or num proxy to that type
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strcmp(t->tag, "sexpr")) { x = lval_sexpr(); }

    for (int i = 0; i < t->children_num; i++){
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

lval* lval_eval(lval* v){
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }

    return v;
}

lval* lval_eval_sexpr(lval* v) {
    // evaluate all the children
    for (int i = 0; i < v->count; i++){
        v->cell[i] = lval_eval(v->cell[i]);
    }

    // error checking
    for (int i = 0; i < v->count; i++){
        if (v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    // empty
    if (v->count == 0) { return v; }

    if (v->count == 1){
        return lval_take(v, 0);
    }

    // ensure first elem is a symbol
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_SYM){
        // dealloc
        lval_del(f); lval_del(v);

        return lval_err("S-expression Does not start with a symbol!");
    }

    // call builtin with operator
    lval* result = builtin_op(v, f->sym);
    lval_del(f);
    return result;
}

lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];

    // shift the memory following the item at "i"" over the top of it
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
    v->count--;

    v->cell = realloc(v->cell, sizeof(lval*) * v->count);

    return x;
}

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}
