#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

/* Tipos de tokens reconocidos por el analizador léxico */
typedef enum {
    TOK_KEYWORD,           /* Palabras reservadas de C */
    TOK_IDENTIFIER,        /* Identificadores */
    TOK_INTEGER_LITERAL,   /* Literales enteros */
    TOK_FLOAT_LITERAL,     /* Literales de punto flotante */
    TOK_CHAR_LITERAL,      /* Literales de carácter */
    TOK_STRING_LITERAL,    /* Literales de cadena */
    TOK_OPERATOR,          /* Operadores */
    TOK_SEPARATOR,         /* Separadores (delimitadores) */
    TOK_WHITESPACE,        /* Espacios en blanco */
    TOK_NEWLINE,           /* Saltos de línea */
    TOK_LEXICAL_ERROR,     /* Errores léxicos */
    TOK_EOF,               /* Fin de archivo */
    TOK_TYPE_COUNT         /* Número total de tipos (usar para dimensionar arreglos) */
} TokenType;

/* Estructura que representa un token individual */
typedef struct {
    TokenType type;        /* Tipo del token */
    char *lexeme;          /* Texto del lexema (memoria dinámica, liberar con token_destroy) */
    long line;             /* Línea donde se encontró */
    long column;           /* Columna donde se encontró */
    double numeric_value;  /* Valor numérico (para literales numéricos) */
} Token;

/* Libera la memoria dinámica asociada a un token */
void token_destroy(Token *token);

/* Retorna el nombre legible del tipo de token */
const char *token_type_name(TokenType type);

/* Crea un token con los valores dados. Duplica el lexema internamente. */
Token token_create(TokenType type, const char *lexeme, long line, long column);

#endif /* TOKEN_H */
