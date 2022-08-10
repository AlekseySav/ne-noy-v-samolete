#pragma once

#include <sys/types.h>

/*
 * 02:49
 * я хз скок лететь бтв
 */

struct object;

/* our only function lol */
void noy(const char* s);
void noy_builtin(const char* name, void (*func)(void));
struct object* noy_param(unsigned n);

#ifdef NOY_IMPL

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

typedef u_int8_t  u8;
typedef u_int16_t u16;
typedef u_int64_t u64;
typedef int64_t i64;
#define STRTAB_SIZE 1024

#pragma region "strtab"

static char* strtab[STRTAB_SIZE];

static u16 lookup(const char* s) {
    u16 p = 0;
    while (strtab[p])
        if (!strcmp(strtab[p++], s))
            return --p;
    strtab[p] = strdup(s);
    return p;
}

#pragma endregion

#pragma region "bytecode"

enum bytecode {
    NOP,

    NOY,
    SYM,
    REF,
    INT,
    STR,
    TUPLE,
    TABLE,
    CALL,

    UNARY,
    NOT = UNARY,
    NEG,
    POP,

    BINARY,
    ADD = BINARY,
    SUB,
    MUL,
    DIV,
    MOD,
    AND,
    OR,
    EQU,
    NEQ,
    GEQ,
    LEQ,
    GES,
    LES,
    IDX,
    SIDX,
    SET,
    CAT,

    /* not opcodes, but tokens */
    CALL0, CALL1,
    IDX0, IDX1,
    TABLE0, TABLE1,
    ENDL,
    KILL, /* ops, opcode, end program */
    /* again not opcodes, object types */
    FUNC,
    EXTRN,
};

/* value prec = 0, postfix prec = 0, prefix op prec = 1, () [] {} don't care */
static u8 prec[] = {
/* what                  {} ()*/
    0, 0, 0, 0, 0, 0, 0, 0, 0,
/* unary */
    1, 1, 1,
/*  +  -  *  /  %  &  |  == != >= <= >  <  [] []= =  , */
    3, 3, 2, 2, 2, 5, 5, 6, 6, 6, 6, 6, 6, 0, 0,  8, 7,
/* braces */
    100, 0, 100, 0, 100, 0
};

u8 program[10000];
u8* pc = program;

#pragma endregion

#pragma region "lexer"

static char ctype[] = {
/* unknown */
    255, 0, 0, 0, 0, 0, 0, 0, 0, 0, ENDL, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, NOT, STR, 0, 0, MOD, AND, STR, CALL0, CALL1, MUL, ADD, CAT, SUB, 0, DIV,
    SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, 0, 0, LES, SET, GES, 0,
    0, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM,
    SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, IDX0, 0, IDX1, 0, SYM,
    0, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM,
    SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, TABLE0, OR, TABLE1, 0, 0
};

static u64 lval;
static int line = 1;
static char c, buf[128], buflen;
static u8 qt = 255;

static char nextchar(void) {
    c = getchar();
    if (c == '\n') line++;
    else if (c == EOF) c = 0;
    return c;
}

static void prevchar(void) { ungetc(c, stdin); line -= c == '\n'; }
static u8 subseq(char to, char s1, char s2) { return to == nextchar() ? s2 : prevchar(), s1; }
static bool endsym(void) { return ctype[c] == SYM ? true : (prevchar(), false); }
static bool endstr(void) { return ctype[c] == STR ? false : true; }

static u8 lex() {
    u8 ct;
    if (qt != 255) return ct = qt, qt = 255, ct;
    bool (*cond)(void);
    buflen = 0;
    while (!ctype[nextchar()]); /* skip unknown */
    switch (ct = ctype[c]) {
        case 255: return 0;
        case GES: return subseq('=', GES, GEQ);
        case LES: return subseq('=', LES, LEQ);
        case SET: return subseq('=', SET, EQU);
        case NOT: return subseq('=', NOT, NEQ);
        case SYM: cond = endsym; break;
        case STR: cond = endstr; nextchar(); break;
        default: return ct;
    }
    while (cond()) (buf[buflen++] = c), c = nextchar();
    buf[buflen] = '\0';
    if (ct == SYM && buf[0] >= '0' && buf[0] <= '9') return lval = atoi(buf), INT;
    lval = lookup(buf);
    if (lval == lookup("noy")) return NOY;
    return ct;
}

#pragma endregion

#pragma region "expr"

/*
 * okk, 03:26
 * херово
 * еще тестить же
 */

/*
 * 03:40
 * aaaaaaaaaaaaaaaaaaaaaa
 * hotya norm
 */

static u8 opstk[100], *s;

static void expr1(u8 end) {
    u8 t, p;
    bool expect = true;
    *++s = 0;
    while ((t = lex()) != end) {
        /* raw value */
        if (t <= STR) {
            expect = true;
            *pc++ = t;
            *(u64*)pc = lval;
            pc += 8;
            continue;
        }
        if ((p = prec[t]) == 100) {
            u8* spc = pc;
            expr1(t + 1);
            if (!expect && t != TABLE0) {
                expect = false;
                continue;
            }
            if (spc == pc) {
                *pc++ = TUPLE;
                *(u64*)pc = 0;
                pc += 8;
            }
            switch (t) {
                case CALL0: t = CALL; break;
                case IDX0: t = IDX; break;
                case TABLE0: t = TABLE; break;
            }
            p = prec[t];
        }
        while (*s && p >= prec[*s])
            *pc++ = *s--;
        *++s = t;
        if (t == SET) {
            if (*(pc - 9) == SYM) *(pc - 9) = REF;
            if (*(pc - 1) == IDX) {
                pc--;
                *s = SIDX;
            }
        }
        expect = false;
    }
    while (*s) *pc++ = *s--;
    s--;
}

static void expr(u8 end) {
    s = opstk;
    expr1(end);
}

/*
 * 04:08
 * okkkkkkkkkkkkkkkk
 */

#pragma endregion

#pragma region "statement"

/*
 * 04:19
 * oke we re good to go
 * wow spent 1:30
 */

static bool statement(void) {
    u8 t = lex();
    if (!t) return false;
    if (t == lookup("fn")) {
        // aaaaaaaaaaaaaaaa
        // fuck, later
    }
    else {
        qt = t;
        expr(ENDL);
        *pc++ = POP;
    }
    return true;
}

#pragma endregion

#pragma region "table"

// table object (04:56)

static struct object {
    u8 type; /* ref ... str */
    union {
        i64 as_int;
        u16 as_str;
        u16 as_ref;
        void (*as_extrn)(void);
        struct table* as_table;
        struct {
            struct object* sp;
            u64 count;
        } as_tuple;
    };
} stack[512], * sp = stack, * bp = stack;

struct table {
    struct entity {
        struct object ref, val;
    } src[512];
};

struct table* table() { return calloc(1, sizeof(struct table)); }

struct object* get(struct table* t, struct object key) {
    struct entity* p = t->src;
    while (p->ref.type) {
        if (p->ref.type == key.type && p->ref.as_int == key.as_int)
            return &p->val;
        p++;
    }
    p->ref = key;
    return &p->val;
}

void set(struct table* t, struct object key, struct object val) {
    *get(t, key) = val;
}

#pragma endregion

#pragma region "ok, ok, interpreter"

static void push(u8 type, u64 val) {
    (++sp)->type = type;
    sp->as_int = val;
}

struct table globals;

static void run(void) {
    u8 op;
    u64 val;
    struct object a, b, c, * ssp;
    sp = stack;
    pc = program;
    for (;;) {
        op = *pc++;
        ssp = sp;
        if (op == KILL) break;
        if (op <= TUPLE) {
            val = *(u64*)pc;
            pc += 8;
            if (op == NOY || op >= REF)
                push(op, val);
        }
        else if (op >= UNARY) {
            a = *sp--;
            if (op >= BINARY) {
                b = a;
                if (a.type != TUPLE)
                    a = *sp--;
                else 
                    a = *(sp - a.as_tuple.count);   /* ooops some serious stack space waste */
            }
        }
        switch (op) {
            case SYM:
                a.as_int = val;
                a.type = STR;
                *++sp = *get(&globals, a);
                break;
            case TABLE:
                sp--;
                push(TABLE, (size_t)table());
                break;
            case CALL:
                if (sp->type != TUPLE) {
                    push(TUPLE, 1);
                    sp->as_tuple.sp = sp - 1;
                    sp->as_tuple.count = 1;
                }
                a = sp[-sp->as_tuple.count - 1];
                push(INT, (size_t)pc);
                push(INT, (size_t)bp);
                bp = sp - 2;
                assert(a.type == EXTRN || a.type == FUNC);
                if (a.type == EXTRN) a.as_extrn();
                break;
            case NOT: push(a.type == NOY ? INT : NOY, 0); break;
            case NEG: push(INT, -a.as_int); break;
            case ADD: push(INT, a.as_int + b.as_int); break;
            case SUB: push(INT, a.as_int + b.as_int); break;
            case MUL: push(INT, a.as_int * b.as_int); break;
            case DIV: push(INT, a.as_int / b.as_int); break;
            case MOD: push(INT, a.as_int % b.as_int); break;
            case AND: push(a.type == NOY || b.type == NOY ? NOY : INT, 0); break;
            case OR:  push(a.type == NOY && b.type == NOY ? NOY : INT, 0); break;
            case EQU: push(a.type == b.type && a.as_int == b.as_int ? INT : NOY, 0); break;
            case NEQ: push(a.type == b.type && a.as_int == b.as_int ? NOY : INT, 0); break;
            case GEQ: push(a.as_int >= b.as_int ? INT : NOY, 0); break;
            case LEQ: push(a.as_int <= b.as_int ? INT : NOY, 0); break;
            case GES: push(a.as_int >  b.as_int ? INT : NOY, 0); break;
            case LES: push(a.as_int <  b.as_int ? INT : NOY, 0); break;
            case IDX: *++sp = *get(a.as_table, b); break;
            case SIDX:
                set(sp->as_table, a, b);
                *sp = b;
                break;
            case SET:
                if (a.type == REF) a.type = STR;
                set(&globals, a, b);
                *++sp = b;
                break;
            case CAT:
                //if (a == )
                //if (a.type == TUPLE) printf("agaaa\n");
                if (a.type == TUPLE) {
                    *++sp = b;
                    *++sp = a;
                }
                else {
                    *++sp = a;
                    *++sp = b;
                    push(TUPLE, 0);
                    sp->as_tuple.sp = sp - 2;
                    sp->as_tuple.count = 1;
                }
                sp->as_tuple.count++;
                break;
        }
    }
}

/*
 * 04:46 done everything except SYM IDX SET CAT TABLE CALL
 */

#pragma endregion

#pragma region "utility"

void noy(const char* s) {
    pc = program;
    FILE* bak = stdin;
    char* dup;
    stdin = fmemopen(dup = strdup(s), strlen(s), "rt");
    while (statement());
    stdin = bak;
    free(dup);
    *pc = KILL;
    run();
}

void noy_builtin(const char* name, void (*func)(void)) {
    push(EXTRN, (size_t)func);
    push(STR, lookup(name));
    set(&globals, *sp, *(sp - 1));
    sp -= 2;
}

struct object* noy_param(unsigned n) {
    assert(bp->type == TUPLE);
    return n >= bp->as_tuple.count ? NULL : bp->as_tuple.sp + n;
}

#pragma endregion

#endif
