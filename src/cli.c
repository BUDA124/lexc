#define _POSIX_C_SOURCE 200809L
#include "cli.h"
#include "util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

void print_usage(FILE *stream, const char *prog_name) {
    fprintf(stream,
        "Uso: %s [opciones] <archivo_entrada>\n"
        "\n"
        "Opciones:\n"
        "  -o <ruta>   Ruta de salida del PDF (por defecto: output/<nombre>.pdf)\n"
        "  -t <ruta>   Ruta de salida del .tex (por defecto: output/<nombre>.tex)\n"
        "  -n          No abrir el visor de PDF al finalizar\n"
        "  -s          Solo preprocesar y escanear (no generar reporte)\n"
        "  -v          Mostrar mensajes de progreso en consola\n"
        "  -h          Mostrar este mensaje de ayuda\n"
        "\n"
        "Ejemplos:\n"
        "  %s programa.c\n"
        "  %s -n -o reporte.pdf fuente.c\n"
        "  %s -h\n"
        "\n"
        "Códigos de retorno:\n"
        "  0   Ejecución exitosa\n"
        "  1   Error en argumentos o ejecución\n",
        prog_name, prog_name, prog_name, prog_name);
}

int parse_arguments(int argc, char **argv, ProgramOptions *options) {
    int opt;
    int got_pdf = 0;
    int got_tex = 0;

    memset(options->pdf_path, 0, sizeof(options->pdf_path));
    memset(options->tex_path, 0, sizeof(options->tex_path));
    options->no_viewer = 0;
    options->show_help = 0;
    options->scan_only = 0;
    options->verbose = 0;
    options->input_path = NULL;
    options->group_members =
        "Carlos Mario Castillo Mena\\\\"
        "Natalia Acuña Alfaro\\\\"
        "Kenneth Rojas Jiménez\\\\"
        "Amanda Ramírez Viales";
    options->course_term = "2026-2";

    optind = 1; /* Reset getopt */

    while ((opt = getopt(argc, argv, "o:t:nsvh")) != -1) {
        switch (opt) {
            case 'o':
                snprintf(options->pdf_path, sizeof(options->pdf_path), "%s", optarg);
                got_pdf = 1;
                break;
            case 't':
                snprintf(options->tex_path, sizeof(options->tex_path), "%s", optarg);
                got_tex = 1;
                break;
            case 'n':
                options->no_viewer = 1;
                break;
            case 's':
                options->scan_only = 1;
                break;
            case 'v':
                options->verbose = 1;
                break;
            case 'h':
                options->show_help = 1;
                break;
            default:
                return -1;
        }
    }

    if (options->show_help) {
        return 0;
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: Falta el archivo de entrada.\n");
        return -1;
    }

    options->input_path = argv[optind];

    /* Extraer el nombre base del archivo de entrada */
    const char *base = strrchr(options->input_path, '/');
    const char *base_win = strrchr(options->input_path, '\\');
    if (base_win && (!base || base_win > base)) base = base_win;
    if (base) base++; else base = options->input_path;

    char sanitized[256];
    sanitize_filename(base, sanitized, sizeof(sanitized));

    if (!got_pdf) {
        snprintf(options->pdf_path, sizeof(options->pdf_path), "output/%s.pdf", sanitized);
    }

    if (!got_tex) {
        snprintf(options->tex_path, sizeof(options->tex_path), "output/%s.tex", sanitized);
    }

    return 0;
}
