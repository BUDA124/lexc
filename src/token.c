#define _POSIX_C_SOURCE 200809L
#include "token.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Token token_create(TokenType type, const char *lexeme, long line, long column) {
    Token tok;
    tok.type = type;
    tok.line = line;
    tok.column = column;
    tok.lexeme = lexeme ? strdup(lexeme) : calloc(1, 1);
    tok.numeric_value = (type == TOK_CONSTAINTEGER || type == TOK_CONSTAFLOAT) ? (lexeme ? atof(lexeme) : 0.0) : 0.0;
    return tok;
}

void token_destroy(Token *token) {
    if (token) {
        if (token->lexeme) {
            free(token->lexeme);
            token->lexeme = NULL;
        }
    }
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOK_IF:                    return "IF";
        case TOK_ELSE:                  return "ELSE";
        case TOK_WHILE:                 return "WHILE";
        case TOK_FOR:                   return "FOR";
        case TOK_RETURN:                return "RETURN";
        case TOK_FLOAT:                 return "FLOAT";
        case TOK_CHAR:                  return "CHAR";
        case TOK_DOUBLE:                return "DOUBLE";
        case TOK_VOID:                  return "VOID";

        case TOK_AUTO:                  return "AUTO";
        case TOK_BREAK:                 return "BREAK";
        case TOK_CASE:                  return "CASE";
        case TOK_CONST:                 return "CONST";
        case TOK_CONTINUE:              return "CONTINUE";
        case TOK_DEFAULT:               return "DEFAULT";
        case TOK_DO:                    return "DO";
        case TOK_ENUM:                  return "ENUM";
        case TOK_EXTERN:                return "EXTERN";
        case TOK_GOTO:                  return "GOTO";
        case TOK_INLINE:                return "INLINE";
        case TOK_INT:                   return "INT";
        case TOK_LONG:                  return "LONG";
        case TOK_REGISTER:              return "REGISTER";
        case TOK_RESTRICT:              return "RESTRICT";
        case TOK_SHORT:                 return "SHORT";
        case TOK_SIGNED:                return "SIGNED";
        case TOK_SIZEOF:                return "SIZEOF";
        case TOK_STATIC:                return "STATIC";
        case TOK_STRUCT:                return "STRUCT";
        case TOK_SWITCH:                return "SWITCH";
        case TOK_TYPEDEF:               return "TYPEDEF";
        case TOK_UNION:                 return "UNION";
        case TOK_UNSIGNED:              return "UNSIGNED";
        case TOK_VOLATILE:              return "VOLATILE";
        case TOK_BOOL:                  return "_BOOL";
        case TOK_COMPLEX:               return "_COMPLEX";
        case TOK_IMAGINARY:             return "_IMAGINARY";
        case TOK_ATOMIC:                return "_ATOMIC";
        case TOK_STATIC_ASSERT:         return "_STATIC_ASSERT";
        case TOK_NORETURN:              return "_NORETURN";
        case TOK_THREAD_LOCAL:          return "_THREAD_LOCAL";
        case TOK_GENERIC:               return "_GENERIC";
        case TOK_ALIGNAS:               return "_ALIGNAS";
        case TOK_ALIGNOF:               return "_ALIGNOF";

        case TOK_IDENTIFICADOR:         return "IDENTIFICADOR";
        case TOK_CONSTAINTEGER:         return "CONSTAINTEGER";
        case TOK_CONSTAFLOAT:           return "CONSTAFLOAT";
        case TOK_CONSTCADENA:           return "CONSTCADENA";
        case TOK_CONSTCHAR:             return "CONSTCHAR";

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

        case TOK_OPMODULO:              return "OPMODULO";
        case TOK_OPINC:                 return "OPINC";
        case TOK_OPDEC:                 return "OPDEC";
        case TOK_OPANDLOGICO:           return "OPANDLOGICO";
        case TOK_OPORLOGICO:            return "OPORLOGICO";
        case TOK_OPANDBITS:             return "OPANDBITS";
        case TOK_OPORBITS:              return "OPORBITS";
        case TOK_OPXORBITS:             return "OPXORBITS";
        case TOK_OPNOTBITS:             return "OPNOTBITS";
        case TOK_OPSHIFTL:              return "OPSHIFTL";
        case TOK_OPSHIFTR:              return "OPSHIFTR";
        case TOK_OPARROW:               return "OPARROW";
        case TOK_OPTERNARIO:            return "OPTERNARIO";
        case TOK_OPMASIGUAL:            return "OPMASIGUAL";
        case TOK_OPMENOSIGUAL:          return "OPMENOSIGUAL";
        case TOK_OPPORIGUAL:            return "OPPORIGUAL";
        case TOK_OPDIVIGUAL:            return "OPDIVIGUAL";
        case TOK_OPMODIGUAL:            return "OPMODIGUAL";
        case TOK_OPANDIGUAL:            return "OPANDIGUAL";
        case TOK_OPORIGUAL:             return "OPORIGUAL";
        case TOK_OPXORIGUAL:            return "OPXORIGUAL";
        case TOK_OPSHIFTLIGUAL:         return "OPSHIFTLIGUAL";
        case TOK_OPSHIFTRIGUAL:         return "OPSHIFTRIGUAL";

        case TOK_LLAVEABIERTA:          return "LLAVEABIERTA";
        case TOK_LLAVECERRADA:          return "LLAVECERRADA";
        case TOK_PARENTESISABIERTA:     return "PARENTESISABIERTA";
        case TOK_PARENTESISCERRADA:     return "PARENTESISCERRADA";
        case TOK_FINSENTENCIA:          return "FINSENTENCIA";
        case TOK_CHARCOMA:              return "CHARCOMA";
        case TOK_CHARPUNTO:             return "CHARPUNTO";
        case TOK_CORCHETEABIERTO:       return "CORCHETEABIERTO";
        case TOK_CORCHETECERRADO:       return "CORCHETECERRADO";
        case TOK_ELIPSIS:               return "ELIPSIS";

        case TOK_WHITESPACE:            return "WHITESPACE";
        case TOK_NEWLINE:               return "NEWLINE";
        case TOK_LEXICAL_ERROR:         return "LEXICAL_ERROR";
        case TOK_EOF:                   return "EOF";
        default:                        return "UNKNOWN";
    }
}

const char *token_category(TokenType type) {
    if (type >= TOK_KEYWORD_START && type <= TOK_KEYWORD_END) {
        return "Palabra Reservada";
    }
    if (type == TOK_IDENTIFICADOR) return "Identificador";
    if (type == TOK_CONSTAINTEGER) return "Número Entero";
    if (type == TOK_CONSTAFLOAT)   return "Número Flotante";
    if (type == TOK_CONSTCADENA)   return "Cadena";
    if (type == TOK_CONSTCHAR)     return "Carácter";
    if (type >= TOK_OP_START && type <= TOK_OP_END)   return "Operador";
    if (type >= TOK_SEP_START && type <= TOK_SEP_END) return "Separador";
    if (type == TOK_LEXICAL_ERROR) return "Error Léxico";
    if (type == TOK_EOF)           return "Fin de Archivo";
    return "Desconocido";
}