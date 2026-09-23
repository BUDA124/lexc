%{
#include "token.h"
#include "scanner_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Variables globales para el escáner
static long currentLine = 1;
static long currentColumn = 1;
static Token currentToken;
static int tokenReady = 0;
static int scannerDone = 0;
static char lastError[256] = {0};

static void update_col(int len) {
    currentColumn += len;
}

// Verifica que un número entero esté dentro del rango permitido
void verificarRango(const char *text) {
    int valor = atoi(text);
    if (valor < -32768 || valor > 32767) {
        snprintf(lastError, sizeof(lastError), "Numero fuera de rango: %d", valor);
    }
}
//macro para crear un token
#define MAKE_TOKEN(t) do { \
    currentToken = token_create((t), yytext, currentLine, currentColumn); \
    update_col(yyleng); \
    tokenReady = 1; \
    return 0; \
} while(0)
%}

/*opciones de Flex*/

%option noyywrap nounput noinput
%option yylineno

/*tokens*/

DIGITO                  [0-9]
LETRA                   [a-zA-Z]
GUIONES                 [-_]
IDENTIFICADOR           {LETRA}({LETRA}|{DIGITO}|{GUIONES})*
CONSTAINTEGER           (-?[1-9][0-9]{0,4}|0)
CONSTAFLOAT             {DIGITO}+\.{DIGITO}+
CONSTCADENA             \"[^\n\"]*\"
OPSUMA                  \+
OPRESTA                 -
OPMULTIPLICACION        \*
OPDIVISION              \/
OPIGUAL                 =
OPMENOR                 <
OPMAYOR                 >
OPMAYORIGUAL            >=
OPMENORIGUAL            <=
OPIGUALDAD              ==
OPDIFERENTE             !=
OPNEGACION              !
OPDOSPUNTOS             :
LLAVEABIERTA            \{
LLAVECERRADA            \}
PARENTESISABIERTA       \(
PARENTESISCERRADA       \)
FINSENTENCIA            ;
CHARCOMA                ,
CHARPUNTO               \.
IF                      if|IF
ELIF                    elif|ELIF
ELSE                    else|ELSE
WHILE                   while|WHILE
FOR                     for|FOR
RETURN                  return|RETURN
INTEGER                 integer|INTEGER
FLOAT                   float|FLOAT
CHAR                    char|CHAR
DOUBLE                  double|DOUBLE
VOID                    void|VOID
DECVAR                  decvar|DECVAR
ENDDEC                  endec|ENDEC
END                     end|END
WRITE                   write|WRITE
READ                    read|READ

%%


{IF}                    { MAKE_TOKEN(TOK_IF); }
{ELIF}                  { MAKE_TOKEN(TOK_ELIF); }
{ELSE}                  { MAKE_TOKEN(TOK_ELSE); }
{WHILE}                 { MAKE_TOKEN(TOK_WHILE); }
{FOR}                   { MAKE_TOKEN(TOK_FOR); }
{RETURN}                { MAKE_TOKEN(TOK_RETURN); }
{INTEGER}               { MAKE_TOKEN(TOK_INTEGER); }
{FLOAT}                 { MAKE_TOKEN(TOK_FLOAT); }
{CHAR}                  { MAKE_TOKEN(TOK_CHAR); }
{DOUBLE}                { MAKE_TOKEN(TOK_DOUBLE); }
{VOID}                  { MAKE_TOKEN(TOK_VOID); }
{DECVAR}                { MAKE_TOKEN(TOK_DECVAR); }
{ENDDEC}                { MAKE_TOKEN(TOK_ENDDEC); }
{END}                   { MAKE_TOKEN(TOK_END); }
{WRITE}                 { MAKE_TOKEN(TOK_WRITE); }
{READ}                  { MAKE_TOKEN(TOK_READ); }

{CONSTAINTEGER}         { verificarRango(yytext); MAKE_TOKEN(TOK_CONSTAINTEGER); }
{CONSTAFLOAT}           { MAKE_TOKEN(TOK_CONSTAFLOAT); }
{CONSTCADENA}           { MAKE_TOKEN(TOK_CONSTCADENA); }

{IDENTIFICADOR}         { MAKE_TOKEN(TOK_IDENTIFICADOR); }

{OPSUMA}                { MAKE_TOKEN(TOK_OPSUMA); }
{OPRESTA}               { MAKE_TOKEN(TOK_OPRESTA); }
{OPMULTIPLICACION}      { MAKE_TOKEN(TOK_OPMULTIPLICACION); }
{OPDIVISION}            { MAKE_TOKEN(TOK_OPDIVISION); }

{OPIGUAL}               { MAKE_TOKEN(TOK_OPIGUAL); }
{OPMAYOR}               { MAKE_TOKEN(TOK_OPMAYOR); }
{OPMENOR}               { MAKE_TOKEN(TOK_OPMENOR); }
{OPMAYORIGUAL}          { MAKE_TOKEN(TOK_OPMAYORIGUAL); }
{OPMENORIGUAL}          { MAKE_TOKEN(TOK_OPMENORIGUAL); }
{OPIGUALDAD}            { MAKE_TOKEN(TOK_OPIGUALDAD); }
{OPDIFERENTE}           { MAKE_TOKEN(TOK_OPDIFERENTE); }
{OPNEGACION}            { MAKE_TOKEN(TOK_OPNEGACION); }
{OPDOSPUNTOS}           { MAKE_TOKEN(TOK_OPDOSPUNTOS); }

{LLAVEABIERTA}          { MAKE_TOKEN(TOK_LLAVEABIERTA); }
{LLAVECERRADA}          { MAKE_TOKEN(TOK_LLAVECERRADA); }
{PARENTESISABIERTA}     { MAKE_TOKEN(TOK_PARENTESISABIERTA); }
{PARENTESISCERRADA}     { MAKE_TOKEN(TOK_PARENTESISCERRADA); }
{FINSENTENCIA}          { MAKE_TOKEN(TOK_FINSENTENCIA); }
{CHARCOMA}              { MAKE_TOKEN(TOK_CHARCOMA); }
{CHARPUNTO}             { MAKE_TOKEN(TOK_CHARPUNTO); }


[ \t]+                  { update_col(yyleng); }


\n                      { currentLine++; currentColumn = 1; }

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


.                       { MAKE_TOKEN(TOK_LEXICAL_ERROR); }


<<EOF>>                 { scannerDone = 1; return 0; }

%%

/*funciones publcias*/

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
