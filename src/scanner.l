/* STUB: Scanner léxico provisional (Persona 3 reemplazará este archivo) */
%{
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "token.h"
#include "scanner_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long current_line = 1;
static long current_col = 1;
static Token current_token;
static int token_ready = 0;
static int scanner_done = 0;
static char last_error[256] = {0};

static void update_col(int len) {
    current_col += len;
}

#define MAKE_TOKEN(t) do { \
    current_token = token_create((t), yytext, current_line, current_col); \
    update_col(yyleng); \
    token_ready = 1; \
    return 0; \
} while(0)

%}

%option noyywrap nounput noinput

KEYWORD     auto|break|case|char|const|continue|default|do|double|else|enum|extern|float|for|goto|if|int|long|register|return|short|signed|sizeof|static|struct|switch|typedef|union|unsigned|void|volatile|while

%%

{KEYWORD}       { MAKE_TOKEN(TOK_KEYWORD); }

[a-zA-Z_][a-zA-Z0-9_]*  { MAKE_TOKEN(TOK_IDENTIFIER); }

[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?  { MAKE_TOKEN(TOK_FLOAT_LITERAL); }
[0-9]+          { MAKE_TOKEN(TOK_INTEGER_LITERAL); }

\'([^'\\]|\\.)\'  { MAKE_TOKEN(TOK_CHAR_LITERAL); }
\"([^"\\]|\\.)*\"  { MAKE_TOKEN(TOK_STRING_LITERAL); }

"++"|"--"|"<<"|">>"|"<="|">="|"=="|"!="|"&&"|"||"|"+="|"-="|"*="|"/="|"%="|"&="|"|="|"^="|"<<="|">>="|"->"  { MAKE_TOKEN(TOK_OPERATOR); }
[+\-*/%=<>!&|^~?:]  { MAKE_TOKEN(TOK_OPERATOR); }

[{}()\[\];,.]   { MAKE_TOKEN(TOK_SEPARATOR); }

\n              { current_line++; current_col = 1; MAKE_TOKEN(TOK_NEWLINE); }
[ \t]+          { MAKE_TOKEN(TOK_WHITESPACE); }

.               { MAKE_TOKEN(TOK_LEXICAL_ERROR); }

<<EOF>>         { scanner_done = 1; return 0; }

%%

int scanner_open(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(last_error, sizeof(last_error), "No se pudo abrir: %s", path);
        return -1;
    }
    yyrestart(f);
    current_line = 1;
    current_col = 1;
    scanner_done = 0;
    token_ready = 0;
    return 0;
}

Token get_token(void) {
    if (scanner_done) {
        return token_create(TOK_EOF, "", current_line, current_col);
    }
    token_ready = 0;
    yylex();
    if (scanner_done) {
        return token_create(TOK_EOF, "", current_line, current_col);
    }
    return current_token;
}

void scanner_close(void) {
    if (yyin && yyin != stdin) {
        fclose(yyin);
        yyin = NULL;
    }
}

const char *scanner_last_error(void) {
    return last_error[0] ? last_error : NULL;
}
