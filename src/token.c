#define _POSIX_C_SOURCE 200809L
#include "token.h"
#include <stdlib.h>
#include <string.h>

void token_destroy(Token *token) {
    if (token && token->lexeme) {
        free(token->lexeme);
        token->lexeme = NULL;
    }
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOK_KEYWORD:         return "Palabra Reservada";
        case TOK_IDENTIFIER:      return "Identificador";
        case TOK_INTEGER_LITERAL: return "Literal Entero";
        case TOK_FLOAT_LITERAL:   return "Literal Flotante";
        case TOK_CHAR_LITERAL:    return "Literal Carácter";
        case TOK_STRING_LITERAL:  return "Literal Cadena";
        case TOK_OPERATOR:        return "Operador";
        case TOK_SEPARATOR:       return "Separador";
        case TOK_WHITESPACE:      return "Espacio en Blanco";
        case TOK_NEWLINE:         return "Salto de Línea";
        case TOK_LEXICAL_ERROR:   return "Error Léxico";
        case TOK_EOF:             return "Fin de Archivo";
        default:                  return "Desconocido";
    }
}

Token token_create(TokenType type, const char *lexeme, long line, long column) {
    Token tok;
    tok.type = type;
    if (lexeme) {
        size_t len = strlen(lexeme);
        tok.lexeme = (char *)malloc(len + 1);
        if (tok.lexeme) {
            memcpy(tok.lexeme, lexeme, len + 1);
        }
    } else {
        tok.lexeme = NULL;
    }
    tok.line = line;
    tok.column = column;
    tok.numeric_value = 0.0;
    return tok;
}
