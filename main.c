#include "main.h"

int main(int argc, char** argv){

    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Symbol  = mpc_new("symbol");
    mpc_parser_t* Sexpr  = mpc_new("sexpr");
    mpc_parser_t* Qexpr  = mpc_new("qexpr");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                       \
            number      : /-?[0-9]+/;                                           \
            symbol      : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/;                     \
            sexpr       : '(' <expr>* ')';                                      \
            qexpr       : '{' <expr>* '}';                                      \
            expr        : <number> | <symbol> | <sexpr> | <qexpr> ;             \
            lispy       : /^/ <expr>* /$/;                                      \
        ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    puts("Lispy Version 0.0.0.2");
    puts("Ctrl+c to Exit\n");

    while(1){
        char* input = readline("lispy> ");
        add_history(input);

        mpc_result_t r;

        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval* x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);
        }
        free(input);
    }

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    return 0;
}

// Print functions
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
        case LVAL_FUN: printf("<function>"); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
        case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    }
}
 void lval_println(lval* v) {
     lval_print(v);
     putchar('\n');
 }

// Lval constructors / manipulation1

void lval_del(lval* v){
    switch(v->type){
        // dont do anything
        case LVAL_NUM:
        case LVAL_FUN:  break;

        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;

        // We need to recurse into this to delete everything
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            for (int i = 0; i < v->count; i++){
                lval_del(v->cell[i]);
            }
            free(v->cell);
           break;
    }

    // free the initial value
    free(v);
}

lval* lval_copy(lval* v){
    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch(v->type){
        // copy numbers and function direclty
        case LVAL_FUN: x->fun = v->fun; break;
        case LVAL_NUM: x->num = v->num; break;

        // allocate and copy symbols and errors
        case LVAL_SYM: x->sym = malloc(strlen(v->sym)+1); strcpy(x->sym, v->sym); break;
        case LVAL_ERR: x->err = malloc(strlen(v->err)+1); strcpy(x->err, v->err); break;

        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for ( int i = 0 ; i < x->count; i++){
                    x->cell[i]  = lval_copy(v->cell[i]);
            }
        break;
    }

    return x;
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

lval* lval_fun(lbuiltin func){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->fun = func;
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

lval* lval_qexpr(void){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
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
    if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

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
    lval* result = builtin(v, f->sym);
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

lval* lval_join(lval* x, lval* y){
    while (y->count){
        x = lval_add(x, lval_pop(y, 0));
    }
    lval_del(y);

    return x;
}

lval* builtin(lval* a, char* func){
  if (strcmp("list", func) == 0) { return builtin_list(a); }
  if (strcmp("head", func) == 0) { return builtin_head(a); }
  if (strcmp("tail", func) == 0) { return builtin_tail(a); }
  if (strcmp("join", func) == 0) { return builtin_join(a); }
  if (strcmp("eval", func) == 0) { return builtin_eval(a); }
  if (strstr("+-/*", func)) { return builtin_op(a, func); }
  lval_del(a);
  return lval_err("Unknown Function!");
}

lval* builtin_op(lval* a, char* op){
    // ensure everything is a number
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM){
            lval_del(a);
            return lval_err("Cannot operate on a non-number");
        }
    }

    lval* x = lval_pop(a, 0);

    // if no arguments and sub preform unary negation
    if ((strcmp(op, "-") == 0 ) && a->count == 0) {
        x->num = -x->num;
    }

    while (a->count > 0){
        lval* y = lval_pop(a, 0);

        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0){
                lval_del(x); lval_del(y);
                x = lval_err("Divisin By Zero!"); break;
            }
            x->num /= y->num;
        }

        // delete finsihed elements
        lval_del(y);
    }

    // delete input expression and return result;
    lval_del(a);

    return x;
}

lval* builtin_head(lval* a) {
    LASSERT(a,(a->count == 1), "Function 'head' passed too many arguments!");
    LASSERT(a,(a->cell[0]->type == LVAL_QEXPR), "Function 'head' passed incorrect type!");
    LASSERT(a,(a->cell[0]->count != 0), "Function 'head' passed {}");

    lval* v = lval_take(a, 0);

    while(v->count > 1) {
        lval_del(lval_pop(v,1));
    }
    return v;
}

lval* builtin_tail(lval* a){
    LASSERT(a,(a->count == 1), "Function 'tail' passed too many arguments!");
    LASSERT(a,(a->cell[0]->type == LVAL_QEXPR), "Function 'tail' passed incorrect type!");
    LASSERT(a,(a->cell[0]->count != 0), "Function 'tail' passed {}");

    lval* v = lval_take(a, 0);
    lval_del(lval_pop(v,0));

    return v;
}

lval* builtin_list(lval* a){
    a->type = LVAL_QEXPR;
    return a;
}

lval* builtin_eval(lval* a){
    LASSERT(a, (a->count == 1), "Function 'eval' passed too many arguments!");
    LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'eval' passed incorrect type!");

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;

    return lval_eval(x);
}

lval* builtin_join(lval* a){
    for (int i = 0; i < a->count; i++){
        LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'eval' passed incorrect type!");
    }

    lval* x = lval_pop(a, 0);

    while(a->count){
        x = lval_join(x, lval_pop(a,0));
    }

    lval_del(a);

    return x;
}

lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));

    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;

    return e;
}

lval* lenv_get(lenv* e, lval* k){
    for (int i = 0; i < e->count; i++){
        // does stored string match symbol? if so return
        if (strcmp(e->syms[i], k->sym) == 0) { return lval_copy(e->vals[i]); }
    }

    return lval_err("unbound symbol");

}

void lenv_put(lenv* e, lval* k, lval* v){
    // check if element exists
    for ( int i = 0; i < e->count; i++){
        // if its found overwrite the current value and replace it with new one
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    // no entry found
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    e->vals[e->count-1] = lval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym) + 1);

    strcpy(e->syms[e->count-1], k->sym);
}

void lenv_del(lenv* e){
    for( int i = 0; i < e->count; i++){
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }

    free(e->syms);
    free(e->vals);
    free(e);
}
