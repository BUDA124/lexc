#ifndef STATS_H
#define STATS_H

#include "token.h"

/* Estructura para acumular estadísticas de tokens */
typedef struct {
    unsigned long counts[TOK_TYPE_COUNT]; /* Conteo por categoría */
    unsigned long total;                   /* Total de tokens procesados */
} TokenStats;

/* Inicializa todos los contadores a cero */
void stats_init(TokenStats *stats);

/* Incrementa el contador correspondiente al tipo del token */
void stats_add(TokenStats *stats, const Token *token);

/* Consulta el conteo de un tipo específico */
unsigned long stats_count(const TokenStats *stats, TokenType type);

/* Imprime un resumen de estadísticas a stdout (para depuración) */
void stats_print(const TokenStats *stats);

#endif /* STATS_H */
