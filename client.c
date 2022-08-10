#define NOY_IMPL
#include "noy.h"
#include <assert.h>

void run_tests(void);

static void print(void) {
    struct object* o;
    unsigned i;
    for (i = 0; o = noy_param(i); i++) {
        switch (o->type) {
            case NOY: printf("yeah sure sure noy yeah again noy"); break;
            case INT: printf("%ld", o->as_int); break;
            case STR: printf("%s", strtab[o->as_str]); break;
            case TUPLE: printf("<tuple<%lu> at %p>", o->as_tuple.count, o->as_tuple.sp); break;
            case TABLE: printf("<table at %p>", o->as_table); break;
            case EXTRN: printf("<extrn at %p>", o->as_extrn); break;
        }
    }
    printf("\n");
}

int main(int argc, char** argv) {
    noy_builtin("print", print);
    run_tests();
    
    char* line; size_t size;
    for(;;) {
        printf("> ");
        getline(&line, &size, stdin);
        if (!strcmp(line, "exit\n")) break;
        noy(line);
    }
}




#define with(in) ({ \
    char buf[1000] = in; \
    stdin = fmemopen(buf, 1000, "rt"); \
})

void run_tests(void) {
    FILE* bak = stdin;

    with("x+8*'hello'/lala\n");
    assert(lex() == SYM && lval == lookup("x"));
    assert(lex() == ADD);
    assert(lex() == INT && lval == 8);
    assert(lex() == MUL);
    assert(lex() == STR && lval == lookup("hello"));
    assert(lex() == DIV);
    assert(lex() == SYM && lval == lookup("lala"));
    assert(lex() == ENDL);
    assert(lex() == 0);

    with("x = y + (4 * 2) & (3(),1)\nnoy\nx[0]=1\n");
    expr(ENDL);
    assert(program[0] == REF);
    assert(*(u64*)&program[1] == lookup("x"));
    assert(program[9] == SYM);
    assert(*(u64*)&program[10] == lookup("y"));
    assert(program[18] == INT);
    assert(*(u64*)&program[19] == 4);
    assert(program[27] == INT);
    assert(*(u64*)&program[28] == 2);
    assert(program[36] == MUL);
    assert(program[37] == ADD);
    assert(program[38] == INT);
    assert(*(u64*)&program[39] == 3);
    assert(program[47] == TUPLE);
    assert(*(u64*)&program[48] == 0);
    assert(program[56] == CALL);
    assert(program[57] == INT);
    assert(*(u64*)&program[58] == 1);
    assert(program[66] == CAT);
    assert(program[67] == AND);
    assert(program[68] == SET);
    expr(ENDL);
    assert(program[69] == NOY);
    expr(ENDL);
    assert(program[78] == SYM);
    assert(program[87] == INT);
    assert(program[96] == INT);
    assert(program[105] == SIDX);

    assert(lookup("x") != lookup("y"));
    struct object a = { .type = STR, .as_str = lookup("x") }, b = { .type = STR, .as_str = lookup("y") };
    assert(get(&globals, a) != get(&globals, b));
    a.as_str = lookup("print");
    assert(get(&globals, a)->type == EXTRN && get(&globals, a)->as_extrn == print);

    noy("x = 1 + 3 * 2\ny=(1,2,3)\n");
    struct object o = { .type = STR };
    o.as_str = lookup("x");
    assert(get(&globals, o)->type == INT && get(&globals, o)->as_int == 7);
    o.as_str = lookup("y");
    assert(get(&globals, o)->type == TUPLE && get(&globals, o)->as_tuple.count == 3 && (get(&globals, o)->as_tuple.sp)->as_int == 1);
    /* wooooooow [ 05:11 ] */

/*
 * ok, 05:53 ????
 * TODO: tuple[+],call[extrn+],builtins,statements
 * 05:54 (?)
 * 06:12
 */
    stdin = bak;
}
