#include "cli.h"
#include "util.h"
#include "preprocessor.h"
#include "scanner_interface.h"
#include "stats.h"
#include "report.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    ProgramOptions options;
    memset(&options, 0, sizeof(options));

    // 1. Parse arguments
    if (parse_arguments(argc, argv, &options) != 0) {
        return EXIT_FAILURE;
    }
    if (options.show_help) {
        print_usage(stdout, argv[0]);
        return EXIT_SUCCESS;
    }

    // 2. Ensure output directory exists
    if (ensure_directory_exists("output") != 0) {
        fprintf(stderr, "Error: No se pudo crear el directorio 'output'.\n");
        return EXIT_FAILURE;
    }

    // 3. Build unique temp path for preprocessed file
    char temp_path[512];
    if (build_temp_path(options.input_path, temp_path, sizeof(temp_path)) != 0) {
        fprintf(stderr, "Error: No se pudo generar la ruta temporal.\n");
        return EXIT_FAILURE;
    }

    // 4. Preprocess
    if (options.verbose) {
        fprintf(stdout, "[1/4] Preprocesando '%s' -> '%s'...\n", options.input_path, temp_path);
    }
    if (preprocess_file(options.input_path, temp_path) != 0) {
        fprintf(stderr, "Error en preprocesamiento: %s\n",
                preprocessor_last_error() ? preprocessor_last_error() : "desconocido");
        return EXIT_FAILURE;
    }

    // 5. Scan tokens
    if (options.verbose) {
        fprintf(stdout, "[2/4] Escaneando tokens...\n");
    }
    if (scanner_open(temp_path) != 0) {
        fprintf(stderr, "Error al abrir scanner: %s\n",
                scanner_last_error() ? scanner_last_error() : "desconocido");
        return EXIT_FAILURE;
    }

    TokenStats stats;
    stats_init(&stats);

    // Dynamic array to accumulate tokens for the report
    size_t token_cap = 256;
    size_t token_count = 0;
    Token *tokens = malloc(token_cap * sizeof(Token));
    if (!tokens) {
        fprintf(stderr, "Error: Sin memoria para almacenar tokens.\n");
        scanner_close();
        return EXIT_FAILURE;
    }

    Token tok;
    do {
        tok = get_token();

        // Store token for report and accumulate stats (except EOF)
        if (tok.type != TOK_EOF) {
            stats_add(&stats, &tok);
            if (token_count >= token_cap) {
                token_cap *= 2;
                Token *tmp = realloc(tokens, token_cap * sizeof(Token));
                if (!tmp) {
                    fprintf(stderr, "Error: Sin memoria al expandir arreglo de tokens.\n");
                    // Cleanup
                    for (size_t i = 0; i < token_count; i++) token_destroy(&tokens[i]);
                    free(tokens);
                    scanner_close();
                    return EXIT_FAILURE;
                }
                tokens = tmp;
            }
            tokens[token_count++] = tok; // Transfer ownership of lexeme
        } else {
            token_destroy(&tok); // Free EOF token's lexeme if any
        }
    } while (tok.type != TOK_EOF);

    scanner_close();

    if (options.verbose) {
        fprintf(stdout, "    Tokens encontrados: %lu\n", (unsigned long)stats.total);
        stats_print(&stats);
    }

    // 7. Generate report (unless scan-only mode)
    if (options.scan_only) {
        if (options.verbose) {
            fprintf(stdout, "[3/3] Modo scan-only: reporte omitido.\n");
        }
    } else {
        if (options.verbose) {
            fprintf(stdout, "[3/4] Generando reporte Beamer -> '%s'...\n", options.pdf_path);
        }
        ReportConfig report_config = {
            .processed_source_path = temp_path,
            .tex_path = options.tex_path,
            .pdf_path = options.pdf_path,
            .group_members = options.group_members,
            .course_term = options.course_term,
            .open_viewer = !options.no_viewer,
            .tokens = tokens,
            .token_count = token_count
        };

        if (generate_beamer_report(&report_config, &stats) != 0) {
            fprintf(stderr, "Error al generar reporte: %s\n",
                    report_last_error() ? report_last_error() : "desconocido");
            // Cleanup
            for (size_t i = 0; i < token_count; i++) token_destroy(&tokens[i]);
            free(tokens);
            return EXIT_FAILURE;
        }

        if (options.verbose) {
            fprintf(stdout, "[4/4] Reporte generado exitosamente: '%s'\n", options.pdf_path);
        }
    }

    // 8. Cleanup
    for (size_t i = 0; i < token_count; i++) {
        token_destroy(&tokens[i]);
    }
    free(tokens);

    return EXIT_SUCCESS;
}
