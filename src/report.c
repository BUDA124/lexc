/* STUB: Implementación provisional del reporte (Persona 4 reemplazará este archivo) */
#include "report.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char last_error[256] = {0};

int generate_beamer_report(const ReportConfig *config, const TokenStats *stats) {
    FILE *f = fopen(config->tex_path, "w");
    if (!f) {
        snprintf(last_error, sizeof(last_error), "No se pudo crear archivo tex: %s", config->tex_path);
        return -1;
    }

    fprintf(f, "\\documentclass{beamer}\n");
    fprintf(f, "\\usepackage[utf8]{inputenc}\n");
    fprintf(f, "\\usepackage{pgfplots}\n");
    fprintf(f, "\\pgfplotsset{compat=1.18}\n");
    fprintf(f, "\\title{Proyecto 1: Analizador Léxico}\n");
    fprintf(f, "\\author{%s}\n", config->group_members ? config->group_members : "Grupo");
    fprintf(f, "\\date{%s}\n", config->course_term ? config->course_term : "Término");
    fprintf(f, "\\begin{document}\n");
    
    fprintf(f, "\\begin{frame}\n");
    fprintf(f, "\\titlepage\n");
    fprintf(f, "\\end{frame}\n");

    fprintf(f, "\\begin{frame}{Resumen de Tokens}\n");
    fprintf(f, "\\begin{tabular}{|l|r|}\n");
    fprintf(f, "\\hline\n");
    fprintf(f, "Tipo & Cantidad \\\\\n");
    fprintf(f, "\\hline\n");
    for (int i = 0; i < TOK_TYPE_COUNT; i++) {
        if (i == TOK_WHITESPACE || i == TOK_NEWLINE) continue;
        fprintf(f, "%s & %lu \\\\\n", token_type_name(i), stats->counts[i]);
    }
    fprintf(f, "\\hline\n");
    fprintf(f, "\\end{tabular}\n");
    fprintf(f, "\\end{frame}\n");

    fprintf(f, "\\begin{frame}{Gráfico de Tokens}\n");
    fprintf(f, "\\begin{tikzpicture}\n");
    fprintf(f, "\\begin{axis}[ybar, enlargelimits=0.15, symbolic x coords={");
    int first = 1;
    for (int i = 0; i < TOK_TYPE_COUNT; i++) {
        if (i == TOK_WHITESPACE || i == TOK_NEWLINE) continue;
        if (!first) fprintf(f, ", ");
        fprintf(f, "%s", token_type_name(i));
        first = 0;
    }
    fprintf(f, "}, xtick=data, x tick label style={rotate=45,anchor=east}]\n");
    fprintf(f, "\\addplot coordinates {");
    for (int i = 0; i < TOK_TYPE_COUNT; i++) {
        if (i == TOK_WHITESPACE || i == TOK_NEWLINE) continue;
        fprintf(f, "(%s, %lu) ", token_type_name(i), stats->counts[i]);
    }
    fprintf(f, "};\n");
    fprintf(f, "\\end{axis}\n");
    fprintf(f, "\\end{tikzpicture}\n");
    fprintf(f, "\\end{frame}\n");
    fprintf(f, "\\end{document}\n");

    fclose(f);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pdflatex -interaction=nonstopmode -output-directory=output %s", config->tex_path);
    system(cmd);
    if (system(cmd) != 0) {
        snprintf(last_error, sizeof(last_error), "Error compilando latex");
        return -1;
    }

    if (config->open_viewer) {
        snprintf(cmd, sizeof(cmd), "xdg-open %s &", config->pdf_path);
        system(cmd);
    }

    return 0;
}

const char *report_last_error(void) {
    return last_error[0] ? last_error : NULL;
}
