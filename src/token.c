#include "token.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//crear token
Token token_create(TokenType type, const char *lexeme, long line, long column) {
    Token tok;
    tok.type = type;
    tok.line = line;
    tok.column = column;
    tok.lexeme = lexeme ? strdup(lexeme) : calloc(1, 1);
    tok.numeric_value = (type == TOK_CONSTAINTEGER || type == TOK_CONSTAFLOAT) ? atof(lexeme) : 0.0;
    
    return tok;
}
//liberar memoria del token
void token_destroy(Token *token) {
    if (token) {
        if (token->lexeme) {
            free(token->lexeme);
            token->lexeme = NULL;
        }
    }
}

//nombre del tipo de token
const char *token_type_name(TokenType type) {
    switch (type) {
        case TOK_IF:                    return "IF";
        case TOK_ELIF:                  return "ELIF";
        case TOK_ELSE:                  return "ELSE";
        case TOK_WHILE:                 return "WHILE";
        case TOK_FOR:                   return "FOR";
        case TOK_RETURN:                return "RETURN";
        case TOK_INTEGER:               return "INTEGER";
        case TOK_FLOAT:                 return "FLOAT";
        case TOK_CHAR:                  return "CHAR";
        case TOK_DOUBLE:                return "DOUBLE";
        case TOK_VOID:                  return "VOID";
        case TOK_DECVAR:                return "DECVAR";
        case TOK_ENDDEC:                return "ENDDEC";
        case TOK_END:                   return "END";
        case TOK_WRITE:                 return "WRITE";
        case TOK_READ:                  return "READ";
        
        case TOK_IDENTIFICADOR:         return "IDENTIFICADOR";
        case TOK_CONSTAINTEGER:         return "CONSTAINTEGER";
        case TOK_CONSTAFLOAT:           return "CONSTAFLOAT";
        case TOK_CONSTCADENA:           return "CONSTCADENA";
        
        case TOK_OPSUMA:                return "OPSUMA";
        case TOK_OPRESTA:               return "OPRESTA";
        case TOK_OPMULTIPLICACION:      return "OPMULTIPLICACION";
        case TOK_OPDIVISION:            return "OPDIVISION";
        case TOK_OPIGUAL:               return "OPIGUAL";
        case TOK_OPMAYOR:               return "OPMAYOR";
        case TOK_OPMENOR:               return "OPMENOR";
        case TOK_OPMAYORIGUAL:          return "OPMAYORIGUAL";
        case TOK_OPMENORIGUAL:          return "OPMENORIGUAL";
        case TOK_OPIGUALDAD:            return "OPIGUALDAD";
        case TOK_OPDIFERENTE:           return "OPDIFERENTE";
        case TOK_OPNEGACION:            return "OPNEGACION";
        case TOK_OPDOSPUNTOS:           return "OPDOSPUNTOS";
        
        case TOK_LLAVEABIERTA:          return "LLAVEABIERTA";
        case TOK_LLAVECERRADA:          return "LLAVECERRADA";
        case TOK_PARENTESISABIERTA:     return "PARENTESISABIERTA";
        case TOK_PARENTESISCERRADA:     return "PARENTESISCERRADA";
        case TOK_FINSENTENCIA:          return "FINSENTENCIA";
        case TOK_CHARCOMA:              return "CHARCOMA";
        case TOK_CHARPUNTO:             return "CHARPUNTO";
        
        case TOK_WHITESPACE:            return "WHITESPACE";
        case TOK_NEWLINE:               return "NEWLINE";
        case TOK_LEXICAL_ERROR:         return "LEXICAL_ERROR";
        case TOK_EOF:                   return "EOF";
        default:                        return "UNKNOWN";
    }
}

//Devuelve la categoría legible del token
const char *token_category(TokenType type) {
    if (type >= TOK_IF && type <= TOK_READ) {
        return "Palabra Reservada";
    }
    if (type == TOK_IDENTIFICADOR) return "Identificador";
    if (type == TOK_CONSTAINTEGER) return "Número Entero";
    if (type == TOK_CONSTAFLOAT) return "Número Flotante";
    if (type == TOK_CONSTCADENA) return "Cadena";
    if (type >= TOK_OPSUMA && type <= TOK_OPDOSPUNTOS) return "Operador";
    if (type >= TOK_LLAVEABIERTA && type <= TOK_CHARPUNTO) return "Separador";
    if (type == TOK_LEXICAL_ERROR) return "Error Léxico";
    if (type == TOK_EOF) return "Fin de Archivo";
    return "Desconocido";
}