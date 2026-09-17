#include "stats.h"
#include <stdio.h>
#include <string.h>

void stats_init(TokenStats *stats) {
    if (!stats) return;
    memset(stats->counts, 0, sizeof(stats->counts));
    stats->total = 0;
}

void stats_add(TokenStats *stats, const Token *token) {
    if (!stats || !token) return;
    if (token->type < TOK_TYPE_COUNT) {
        stats->counts[token->type]++;
        stats->total++;
    }
}

unsigned long stats_count(const TokenStats *stats, TokenType type) {
    if (!stats) return 0;
    if (type < TOK_TYPE_COUNT) {
        return stats->counts[type];
    }
    return 0;
}

void stats_print(const TokenStats *stats) {
    if (!stats) return;
    printf("Estadísticas de tokens:\n");
    printf("----------------------------------------\n");
    for (int i = 0; i < TOK_TYPE_COUNT; i++) {
        if (stats->counts[i] > 0) {
            printf("%-25s : %lu\n", token_type_name((TokenType)i), (unsigned long)stats->counts[i]);
        }
    }
    printf("----------------------------------------\n");
    printf("%-25s : %lu\n", "TOTAL", (unsigned long)stats->total);
}
