#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

// Tipo de tokens
typedef enum {
    // Palabras reservadas
    TOK_IF,
    TOK_ELIF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_RETURN,
    TOK_INTEGER,
    TOK_FLOAT,
    TOK_CHAR,
    TOK_DOUBLE,
    TOK_VOID,
    TOK_DECVAR,
    TOK_ENDDEC,
    TOK_END,
    TOK_WRITE,
    TOK_READ,
    
    //Identificadores y constantes
    TOK_IDENTIFICADOR,
    TOK_CONSTAINTEGER,
    TOK_CONSTAFLOAT,
    TOK_CONSTCADENA,
    
    // Operadores
    TOK_OPSUMA,
    TOK_OPRESTA,
    TOK_OPMULTIPLICACION,
    TOK_OPDIVISION,
    TOK_OPIGUAL,
    TOK_OPMAYOR,
    TOK_OPMENOR,
    TOK_OPMAYORIGUAL,
    TOK_OPMENORIGUAL,
    TOK_OPIGUALDAD,
    TOK_OPDIFERENTE,
    TOK_OPNEGACION,
    TOK_OPDOSPUNTOS,
    
    //Separadores
    TOK_LLAVEABIERTA,
    TOK_LLAVECERRADA,
    TOK_PARENTESISABIERTA,
    TOK_PARENTESISCERRADA,
    TOK_FINSENTENCIA,
    TOK_CHARCOMA,
    TOK_CHARPUNTO,
    
    //Especiales
    TOK_WHITESPACE,
    TOK_NEWLINE,
    TOK_LEXICAL_ERROR,
    TOK_EOF,
    TOK_TYPE_COUNT
} TokenType;


// Estructura del token
typedef struct {
    TokenType type;
    char *lexeme;
    long line;
    long column;
    double numeric_value;
} Token;


//funciones publicas
Token token_create(TokenType type, const char *lexeme, long line, long column);
void token_destroy(Token *token);
const char *token_type_name(TokenType type);
const char *token_category(TokenType type);

#endif 