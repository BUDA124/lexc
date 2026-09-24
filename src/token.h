#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

// Tipo de tokens
typedef enum {
    // === Palabras reservadas (inicio de categoría Palabras Clave) ===
    TOK_KEYWORD_START,
    TOK_IF = TOK_KEYWORD_START,
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
    // Palabras reservadas de C estándar adicionales
    TOK_AUTO,
    TOK_BREAK,
    TOK_CASE,
    TOK_CONST,
    TOK_CONTINUE,
    TOK_DEFAULT,
    TOK_DO,
    TOK_ENUM,
    TOK_EXTERN,
    TOK_GOTO,
    TOK_INLINE,
    TOK_INT,
    TOK_LONG,
    TOK_REGISTER,
    TOK_RESTRICT,
    TOK_SHORT,
    TOK_SIGNED,
    TOK_SIZEOF,
    TOK_STATIC,
    TOK_STRUCT,
    TOK_SWITCH,
    TOK_TYPEDEF,
    TOK_UNION,
    TOK_UNSIGNED,
    TOK_VOLATILE,
    TOK_BOOL,
    TOK_COMPLEX,
    TOK_IMAGINARY,
    TOK_ATOMIC,
    TOK_STATIC_ASSERT,
    TOK_NORETURN,
    TOK_THREAD_LOCAL,
    TOK_GENERIC,
    TOK_ALIGNAS,
    TOK_ALIGNOF,
    TOK_KEYWORD_END = TOK_ALIGNOF,

    // === Identificadores y constantes ===
    TOK_IDENTIFICADOR,
    TOK_CONSTAINTEGER,
    TOK_CONSTAFLOAT,
    TOK_CONSTCADENA,
    TOK_CONSTCHAR,

    // === Operadores (inicio de categoría Operadores) ===
    TOK_OP_START,
    TOK_OPSUMA = TOK_OP_START,
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
    // Operadores adicionales de C
    TOK_OPMODULO,         // %
    TOK_OPINC,            // ++
    TOK_OPDEC,            // --
    TOK_OPANDLOGICO,      // &&
    TOK_OPORLOGICO,       // ||
    TOK_OPANDBITS,        // &
    TOK_OPORBITS,         // |
    TOK_OPXORBITS,        // ^
    TOK_OPNOTBITS,        // ~
    TOK_OPSHIFTL,         // <<
    TOK_OPSHIFTR,         // >>
    TOK_OPARROW,          // ->
    TOK_OPTERNARIO,       // ?
    TOK_OPMASIGUAL,       // +=
    TOK_OPMENOSIGUAL,     // -=
    TOK_OPPORIGUAL,       // *=
    TOK_OPDIVIGUAL,       // /=
    TOK_OPMODIGUAL,       // %=
    TOK_OPANDIGUAL,       // &=
    TOK_OPORIGUAL,        // |=
    TOK_OPXORIGUAL,       // ^=
    TOK_OPSHIFTLIGUAL,    // <<=
    TOK_OPSHIFTRIGUAL,    // >>=
    TOK_OP_END = TOK_OPSHIFTRIGUAL,

    // === Separadores (inicio de categoría Separadores) ===
    TOK_SEP_START,
    TOK_LLAVEABIERTA = TOK_SEP_START,
    TOK_LLAVECERRADA,
    TOK_PARENTESISABIERTA,
    TOK_PARENTESISCERRADA,
    TOK_FINSENTENCIA,
    TOK_CHARCOMA,
    TOK_CHARPUNTO,
    // Separadores adicionales de C
    TOK_CORCHETEABIERTO,  // [
    TOK_CORCHETECERRADO,  // ]
    TOK_ELIPSIS,          // ...
    TOK_SEP_END = TOK_ELIPSIS,

    // === Especiales ===
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

// Funciones públicas
Token token_create(TokenType type, const char *lexeme, long line, long column);
void token_destroy(Token *token);
const char *token_type_name(TokenType type);
const char *token_category(TokenType type);

#endif /* TOKEN_H */