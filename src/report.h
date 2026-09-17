#ifndef REPORT_H
#define REPORT_H

#include "stats.h"
#include "token.h"

/*
 * Contrato del módulo de generación de reporte Beamer/PDF (Persona 4).
 *
 * Responsabilidades:
 * - Generar un archivo .tex con presentación Beamer
 * - Incluir portada con datos del equipo y fecha
 * - Mostrar el código fuente preprocesado con coloreado por token
 * - Incluir tabla resumen de tokens por categoría
 * - Generar gráfico de pastel y barras con pgfplots
 * - Compilar el .tex a PDF con pdflatex
 */

/* Configuración del reporte */
typedef struct {
    const char *processed_source_path; /* Ruta al archivo preprocesado */
    const char *tex_path;              /* Ruta de salida del .tex */
    const char *pdf_path;              /* Ruta de salida del .pdf */
    const char *group_members;         /* Nombres de integrantes */
    const char *course_term;           /* Semestre / periodo */
    int open_viewer;                   /* 1 = abrir visor, 0 = no abrir */
    const Token *tokens;               /* Arreglo de tokens para coloreado */
    size_t token_count;                /* Número de tokens en el arreglo */
} ReportConfig;

/* Genera el reporte Beamer y compila a PDF.
 * Retorna 0 en éxito, -1 en error. */
int generate_beamer_report(const ReportConfig *config, const TokenStats *stats);

/* Retorna un mensaje descriptivo del último error, o NULL. */
const char *report_last_error(void);

#endif /* REPORT_H */
