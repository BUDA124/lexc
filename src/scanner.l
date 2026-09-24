%{
#include "token.h"
#include "scanner_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long currentLine = 1;
static long currentColumn = 1;
static Token currentToken;
static int tokenReady = 0;
static int scannerDone = 0;
static char lastError[256] = {0};

static void update_col(int len) {
    currentColumn += len;
}

#define MAKE_TOKEN(t) do { \
    currentToken = token_create((t), yytext, currentLine, currentColumn); \
    update_col(yyleng); \
    tokenReady = 1; \
    return 0; \
} while(0)
%}

%option noyywrap nounput noinput
%option yylineno

/* Prefijos y componentes */
DIGITO                  [0-9]
HEXDIGITO               [0-9a-fA-F]
OCTDIGITO               [0-7]
LETRA                   [a-zA-Z_]
INTSUFFIX               ([uU]ll|[uU]LL|ll[uU]|LL[uU]|[uU][lL]|[lL][uU]|[uU]|[lL])
FLOATSUFFIX             [fFlL]
EXPONENTE               ([eE][+-]?[0-9]+)
PEXPONENTE              ([pP][+-]?[0-9]+)

/* Identificador C estándar */
IDENTIFICADOR           {LETRA}({LETRA}|{DIGITO})*

/* Constantes enteras */
HEXINTEGER              0[xX]{HEXDIGITO}+{INTSUFFIX}?
OCTINTEGER              0{OCTDIGITO}+{INTSUFFIX}?
DECINTEGER              (0|[1-9]{DIGITO}*){INTSUFFIX}?

/* Constantes flotantes */
FLOAT1                  {DIGITO}+\.{DIGITO}*{EXPONENTE}?{FLOATSUFFIX}?
FLOAT2                  \.{DIGITO}+{EXPONENTE}?{FLOATSUFFIX}?
FLOAT3                  {DIGITO}+{EXPONENTE}{FLOATSUFFIX}?
FLOAT_HEX               0[xX]{HEXDIGITO}*\.{HEXDIGITO}+{PEXPONENTE}{FLOATSUFFIX}?

/* Literales de cadena y carácter */
CONSTCADENA             \"([^"\\\n]|\\.)*\"
CONSTCHAR               \'([^'\\\n]|\\.)+\'

%%

 /* ================= Palabras Reservadas ================= */
"auto"                  { MAKE_TOKEN(TOK_AUTO); }
"break"                 { MAKE_TOKEN(TOK_BREAK); }
"case"                  { MAKE_TOKEN(TOK_CASE); }
"char"                  { MAKE_TOKEN(TOK_CHAR); }
"const"                 { MAKE_TOKEN(TOK_CONST); }
"continue"              { MAKE_TOKEN(TOK_CONTINUE); }
"default"               { MAKE_TOKEN(TOK_DEFAULT); }
"do"                    { MAKE_TOKEN(TOK_DO); }
"double"                { MAKE_TOKEN(TOK_DOUBLE); }
"else"                  { MAKE_TOKEN(TOK_ELSE); }
"enum"                  { MAKE_TOKEN(TOK_ENUM); }
"extern"                { MAKE_TOKEN(TOK_EXTERN); }
"float"                 { MAKE_TOKEN(TOK_FLOAT); }
"for"                   { MAKE_TOKEN(TOK_FOR); }
"goto"                  { MAKE_TOKEN(TOK_GOTO); }
"if"                    { MAKE_TOKEN(TOK_IF); }
"inline"                { MAKE_TOKEN(TOK_INLINE); }
"int"                   { MAKE_TOKEN(TOK_INT); }
"long"                  { MAKE_TOKEN(TOK_LONG); }
"register"              { MAKE_TOKEN(TOK_REGISTER); }
"restrict"              { MAKE_TOKEN(TOK_RESTRICT); }
"return"                { MAKE_TOKEN(TOK_RETURN); }
"short"                 { MAKE_TOKEN(TOK_SHORT); }
"signed"                { MAKE_TOKEN(TOK_SIGNED); }
"sizeof"                { MAKE_TOKEN(TOK_SIZEOF); }
"static"                { MAKE_TOKEN(TOK_STATIC); }
"struct"                { MAKE_TOKEN(TOK_STRUCT); }
"switch"                { MAKE_TOKEN(TOK_SWITCH); }
"typedef"               { MAKE_TOKEN(TOK_TYPEDEF); }
"union"                 { MAKE_TOKEN(TOK_UNION); }
"unsigned"              { MAKE_TOKEN(TOK_UNSIGNED); }
"void"                  { MAKE_TOKEN(TOK_VOID); }
"volatile"              { MAKE_TOKEN(TOK_VOLATILE); }
"while"                 { MAKE_TOKEN(TOK_WHILE); }

"_Alignas"              { MAKE_TOKEN(TOK_ALIGNAS); }
"_Alignof"              { MAKE_TOKEN(TOK_ALIGNOF); }
"_Atomic"               { MAKE_TOKEN(TOK_ATOMIC); }
"_Bool"                 { MAKE_TOKEN(TOK_BOOL); }
"_Complex"              { MAKE_TOKEN(TOK_COMPLEX); }
"_Generic"              { MAKE_TOKEN(TOK_GENERIC); }
"_Imaginary"            { MAKE_TOKEN(TOK_IMAGINARY); }
"_Noreturn"             { MAKE_TOKEN(TOK_NORETURN); }
"_Static_assert"        { MAKE_TOKEN(TOK_STATIC_ASSERT); }
"_Thread_local"         { MAKE_TOKEN(TOK_THREAD_LOCAL); }

 /* Retrocompatibilidad con lenguaje didáctico */
(?i:elif)               { MAKE_TOKEN(TOK_ELIF); }
(?i:integer)            { MAKE_TOKEN(TOK_INTEGER); }
(?i:decvar)             { MAKE_TOKEN(TOK_DECVAR); }
(?i:endec)              { MAKE_TOKEN(TOK_ENDDEC); }
(?i:end)                { MAKE_TOKEN(TOK_END); }
(?i:write)              { MAKE_TOKEN(TOK_WRITE); }
(?i:read)               { MAKE_TOKEN(TOK_READ); }
"IF"                    { MAKE_TOKEN(TOK_IF); }
"ELSE"                  { MAKE_TOKEN(TOK_ELSE); }
"WHILE"                 { MAKE_TOKEN(TOK_WHILE); }
"FOR"                   { MAKE_TOKEN(TOK_FOR); }
"RETURN"                { MAKE_TOKEN(TOK_RETURN); }
"FLOAT"                 { MAKE_TOKEN(TOK_FLOAT); }
"CHAR"                  { MAKE_TOKEN(TOK_CHAR); }
"DOUBLE"                { MAKE_TOKEN(TOK_DOUBLE); }
"VOID"                  { MAKE_TOKEN(TOK_VOID); }

 /* ================= Constantes y Literales ================= */
{HEXINTEGER}            { MAKE_TOKEN(TOK_CONSTAINTEGER); }
{OCTINTEGER}            { MAKE_TOKEN(TOK_CONSTAINTEGER); }
{DECINTEGER}            { MAKE_TOKEN(TOK_CONSTAINTEGER); }

{FLOAT1}                { MAKE_TOKEN(TOK_CONSTAFLOAT); }
{FLOAT2}                { MAKE_TOKEN(TOK_CONSTAFLOAT); }
{FLOAT3}                { MAKE_TOKEN(TOK_CONSTAFLOAT); }
{FLOAT_HEX}             { MAKE_TOKEN(TOK_CONSTAFLOAT); }

{CONSTCADENA}           { MAKE_TOKEN(TOK_CONSTCADENA); }
{CONSTCHAR}             { MAKE_TOKEN(TOK_CONSTCHAR); }

 /* ================= Identificadores ================= */
{IDENTIFICADOR}         { MAKE_TOKEN(TOK_IDENTIFICADOR); }

 /* ================= Operadores y Puntuadores ================= */
 /* Operadores de 3 caracteres */
"..."                   { MAKE_TOKEN(TOK_ELIPSIS); }
">>="                   { MAKE_TOKEN(TOK_OPSHIFTRIGUAL); }
"<<="                   { MAKE_TOKEN(TOK_OPSHIFTLIGUAL); }

 /* Operadores de 2 caracteres */
"->"                    { MAKE_TOKEN(TOK_OPARROW); }
"++"                    { MAKE_TOKEN(TOK_OPINC); }
"--"                    { MAKE_TOKEN(TOK_OPDEC); }
"<<"                    { MAKE_TOKEN(TOK_OPSHIFTL); }
">>"                    { MAKE_TOKEN(TOK_OPSHIFTR); }
"<="                    { MAKE_TOKEN(TOK_OPMENORIGUAL); }
">="                    { MAKE_TOKEN(TOK_OPMAYORIGUAL); }
"=="                    { MAKE_TOKEN(TOK_OPIGUALDAD); }
"!="                    { MAKE_TOKEN(TOK_OPDIFERENTE); }
"&&"                    { MAKE_TOKEN(TOK_OPANDLOGICO); }
"||"                    { MAKE_TOKEN(TOK_OPORLOGICO); }
"*="                    { MAKE_TOKEN(TOK_OPPORIGUAL); }
"/="                    { MAKE_TOKEN(TOK_OPDIVIGUAL); }
"%="                    { MAKE_TOKEN(TOK_OPMODIGUAL); }
"+="                    { MAKE_TOKEN(TOK_OPMASIGUAL); }
"-="                    { MAKE_TOKEN(TOK_OPMENOSIGUAL); }
"&="                    { MAKE_TOKEN(TOK_OPANDIGUAL); }
"^="                    { MAKE_TOKEN(TOK_OPXORIGUAL); }
"|="                    { MAKE_TOKEN(TOK_OPORIGUAL); }

 /* Operadores y separadores de 1 carácter */
";"                     { MAKE_TOKEN(TOK_FINSENTENCIA); }
"{"                     { MAKE_TOKEN(TOK_LLAVEABIERTA); }
"}"                     { MAKE_TOKEN(TOK_LLAVECERRADA); }
","                     { MAKE_TOKEN(TOK_CHARCOMA); }
":"                     { MAKE_TOKEN(TOK_OPDOSPUNTOS); }
"="                     { MAKE_TOKEN(TOK_OPIGUAL); }
"("                     { MAKE_TOKEN(TOK_PARENTESISABIERTA); }
")"                     { MAKE_TOKEN(TOK_PARENTESISCERRADA); }
"["                     { MAKE_TOKEN(TOK_CORCHETEABIERTO); }
"]"                     { MAKE_TOKEN(TOK_CORCHETECERRADO); }
"."                     { MAKE_TOKEN(TOK_CHARPUNTO); }
"&"                     { MAKE_TOKEN(TOK_OPANDBITS); }
"!"                     { MAKE_TOKEN(TOK_OPNEGACION); }
"~"                     { MAKE_TOKEN(TOK_OPNOTBITS); }
"-"                     { MAKE_TOKEN(TOK_OPRESTA); }
"+"                     { MAKE_TOKEN(TOK_OPSUMA); }
"*"                     { MAKE_TOKEN(TOK_OPMULTIPLICACION); }
"/"                     { MAKE_TOKEN(TOK_OPDIVISION); }
"%"                     { MAKE_TOKEN(TOK_OPMODULO); }
"<"                     { MAKE_TOKEN(TOK_OPMENOR); }
">"                     { MAKE_TOKEN(TOK_OPMAYOR); }
"^"                     { MAKE_TOKEN(TOK_OPXORBITS); }
"|"                     { MAKE_TOKEN(TOK_OPORBITS); }
"?"                     { MAKE_TOKEN(TOK_OPTERNARIO); }

 /* ================= Espacios y Saltos ================= */
[ \t\r\v\f]+            { update_col(yyleng); }
\n                      { currentLine++; currentColumn = 1; }

 /* ================= Comentarios (fallback) ================= */
\/\*(.|\n)*?\*\/        {
    for (int i = 0; yytext[i]; i++) {
        if (yytext[i] == '\n') {
            currentLine++;
            currentColumn = 1;
        } else {
            currentColumn++;
        }
    }
}
\/\/[^\n]*              { currentColumn += yyleng; }

 /* ================= Errores Léxicos ================= */
.                       { MAKE_TOKEN(TOK_LEXICAL_ERROR); }

<<EOF>>                 { scannerDone = 1; return 0; }

%%

int scanner_open(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(lastError, sizeof(lastError), "No se pudo abrir: %s", path);
        return -1;
    }
    yyrestart(f);
    currentLine = 1;
    currentColumn = 1;
    scannerDone = 0;
    tokenReady = 0;
    return 0;
}

Token get_token(void) {
    if (scannerDone) {
        return token_create(TOK_EOF, "", currentLine, currentColumn);
    }
    tokenReady = 0;
    yylex();
    if (scannerDone) {
        return token_create(TOK_EOF, "", currentLine, currentColumn);
    }
    return currentToken;
}

void scanner_close(void) {
    if (yyin && yyin != stdin) {
        fclose(yyin);
        yyin = NULL;
    }
    yylex_destroy();
}

const char *scanner_last_error(void) {
    return lastError[0] ? lastError : NULL;
}
