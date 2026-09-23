/*
 * preprocessor.c — Implementación completa del preprocesador.
 *
 * Responsabilidades:
 *   - Eliminar comentarios de línea y de bloque
 *   - Expandir directivas de include (archivos locales, cualquier extensión)
 *   - Expandir define simples (sustitución textual sin parámetros)
 *   - Conservar la estructura de líneas para posiciones correctas del scanner
 */
#define _POSIX_C_SOURCE 200809L
#include "preprocessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ======================== Configuración ======================== */
#define MAX_DEFINES         256
#define MAX_DEFINE_NAME     256
#define MAX_DEFINE_VAL      1024
#define MAX_LINE            8192
#define MAX_INCLUDE_DEPTH   16
#define MAX_PATH_LEN        512

/* ======================== Estado interno ======================== */
static char last_error[512] = {0};

typedef struct {
    char name[MAX_DEFINE_NAME];
    char value[MAX_DEFINE_VAL];
} DefineMacro;

typedef struct {
    DefineMacro defines[MAX_DEFINES];
    int define_count;

    char include_stack[MAX_INCLUDE_DEPTH][MAX_PATH_LEN];
    int include_depth;

    int in_block_comment;      /* 1 si estamos dentro de un comentario de bloque */
} PPState;

static PPState pp;

/* ======================== Utilidades ======================== */

/* Extrae la parte de directorio de una ruta (sin el último separador) */
static void path_dirname(const char *path, char *dir, size_t dir_size) {
    const char *sep = strrchr(path, '/');
    const char *sep_win = strrchr(path, '\\');
    if (sep_win && (!sep || sep_win > sep)) sep = sep_win;

    if (sep) {
        size_t len = (size_t)(sep - path);
        if (len >= dir_size) len = dir_size - 1;
        memcpy(dir, path, len);
        dir[len] = '\0';
    } else {
        if (dir_size > 1) { dir[0] = '.'; dir[1] = '\0'; }
        else if (dir_size == 1) { dir[0] = '\0'; }
    }
}

/* ¿El carácter c es válido dentro de un identificador? */
static int is_ident(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

/* ======================== Eliminación de comentarios ======================== */

/*
 * Procesa una línea eliminando comentarios, respetando literales string y char.
 * Actualiza pp.in_block_comment para comentarios de bloque multilínea.
 * Preserva '\n' dentro de comentarios de bloque para mantener conteo de líneas.
 */
static void strip_comments(const char *line, char *out, size_t out_size) {
    size_t i = 0, o = 0;
    size_t len = strlen(line);

    while (i < len && o < out_size - 1) {
        if (pp.in_block_comment) {
            /* Dentro de comentario de bloque: buscar cierre */
            if (line[i] == '*' && i + 1 < len && line[i + 1] == '/') {
                pp.in_block_comment = 0;
                i += 2;
            } else {
                /* Preservar saltos de línea para mantener numeración */
                if (line[i] == '\n') {
                    out[o++] = '\n';
                }
                i++;
            }
        } else if (line[i] == '"') {
            /* String literal — copiar íntegro */
            out[o++] = line[i++];
            while (i < len && o < out_size - 1) {
                out[o++] = line[i];
                if (line[i] == '\\' && i + 1 < len) {
                    i++;
                    if (o < out_size - 1) out[o++] = line[i++];
                } else if (line[i] == '"') {
                    i++;
                    break;
                } else if (line[i] == '\n') {
                    i++;
                    break;
                } else {
                    i++;
                }
            }
        } else if (line[i] == '\'') {
            /* Literal de carácter — copiar íntegro */
            out[o++] = line[i++];
            while (i < len && o < out_size - 1) {
                out[o++] = line[i];
                if (line[i] == '\\' && i + 1 < len) {
                    i++;
                    if (o < out_size - 1) out[o++] = line[i++];
                } else if (line[i] == '\'') {
                    i++;
                    break;
                } else {
                    i++;
                }
            }
        } else if (line[i] == '/' && i + 1 < len && line[i + 1] == '/') {
            /* Comentario de línea — saltar hasta el final, preservar \n */
            while (i < len && line[i] != '\n') i++;
            /* El '\n' se añadirá en la siguiente iteración o al final */
        } else if (line[i] == '/' && i + 1 < len && line[i + 1] == '*') {
            /* Inicio de comentario de bloque */
            pp.in_block_comment = 1;
            i += 2;
        } else {
            out[o++] = line[i++];
        }
    }
    out[o] = '\0';
}

/* ======================== Manejo de #define ======================== */

/* Almacena un #define. Permite redefinición. */
static int store_define(const char *name, const char *value) {
    /* Buscar redefinición */
    for (int d = 0; d < pp.define_count; d++) {
        if (strcmp(pp.defines[d].name, name) == 0) {
            strncpy(pp.defines[d].value, value, MAX_DEFINE_VAL - 1);
            pp.defines[d].value[MAX_DEFINE_VAL - 1] = '\0';
            return 0;
        }
    }

    if (pp.define_count >= MAX_DEFINES) {
        snprintf(last_error, sizeof(last_error),
                 "Demasiados #define (límite: %d)", MAX_DEFINES);
        return -1;
    }

    strncpy(pp.defines[pp.define_count].name, name, MAX_DEFINE_NAME - 1);
    pp.defines[pp.define_count].name[MAX_DEFINE_NAME - 1] = '\0';
    strncpy(pp.defines[pp.define_count].value, value, MAX_DEFINE_VAL - 1);
    pp.defines[pp.define_count].value[MAX_DEFINE_VAL - 1] = '\0';
    pp.define_count++;
    return 0;
}

/*
 * Aplica todas las macros #define a una línea.
 * Realiza sustitución de palabras completas (no reemplaza prefijos).
 * No sustituye dentro de literales string o char.
 * Soporta expansión encadenada (múltiples pasadas, máx. 10).
 */
static void apply_defines(const char *line, char *out, size_t out_size) {
    if (pp.define_count == 0) {
        strncpy(out, line, out_size);
        out[out_size - 1] = '\0';
        return;
    }

    char current[MAX_LINE], result[MAX_LINE];
    strncpy(current, line, MAX_LINE - 1);
    current[MAX_LINE - 1] = '\0';

    for (int pass = 0; pass < 10; pass++) {
        int any_replaced = 0;
        size_t o = 0;
        size_t len = strlen(current);

        for (size_t i = 0; i < len && o < MAX_LINE - 2; ) {
            /* String literal — copiar sin sustituir */
            if (current[i] == '"') {
                result[o++] = current[i++];
                while (i < len && o < MAX_LINE - 2) {
                    result[o++] = current[i];
                    if (current[i] == '\\' && i + 1 < len) {
                        i++;
                        if (o < MAX_LINE - 2) result[o++] = current[i++];
                    } else if (current[i] == '"') {
                        i++;
                        break;
                    } else {
                        i++;
                    }
                }
                continue;
            }

            /* Char literal — copiar sin sustituir */
            if (current[i] == '\'') {
                result[o++] = current[i++];
                while (i < len && o < MAX_LINE - 2) {
                    result[o++] = current[i];
                    if (current[i] == '\\' && i + 1 < len) {
                        i++;
                        if (o < MAX_LINE - 2) result[o++] = current[i++];
                    } else if (current[i] == '\'') {
                        i++;
                        break;
                    } else {
                        i++;
                    }
                }
                continue;
            }

            /* Identificador — verificar si es un macro definido */
            if (is_ident(current[i]) && !isdigit((unsigned char)current[i])) {
                size_t id_start = i;
                while (i < len && is_ident(current[i])) i++;
                size_t id_len = i - id_start;

                int found = 0;
                for (int d = 0; d < pp.define_count; d++) {
                    size_t name_len = strlen(pp.defines[d].name);
                    if (id_len == name_len &&
                        strncmp(current + id_start, pp.defines[d].name, name_len) == 0) {
                        /* Sustituir */
                        size_t val_len = strlen(pp.defines[d].value);
                        if (o + val_len < MAX_LINE - 1) {
                            memcpy(result + o, pp.defines[d].value, val_len);
                            o += val_len;
                        }
                        found = 1;
                        any_replaced = 1;
                        break;
                    }
                }
                if (!found) {
                    /* Copiar el identificador sin cambios */
                    if (o + id_len < MAX_LINE - 1) {
                        memcpy(result + o, current + id_start, id_len);
                        o += id_len;
                    }
                }
                continue;
            }

            /* Cualquier otro carácter */
            result[o++] = current[i++];
        }
        result[o] = '\0';

        if (!any_replaced) break;
        strcpy(current, result);
    }

    strncpy(out, result, out_size);
    out[out_size - 1] = '\0';
}

/* ======================== Manejo de #include ======================== */

/* Verifica si una ruta ya está en el stack de inclusiones (circular) */
static int is_circular(const char *path) {
    for (int i = 0; i < pp.include_depth; i++) {
        if (strcmp(pp.include_stack[i], path) == 0) {
            return 1;
        }
    }
    return 0;
}

static int push_include(const char *path) {
    if (pp.include_depth >= MAX_INCLUDE_DEPTH) {
        snprintf(last_error, sizeof(last_error),
                 "Profundidad máxima de #include excedida (%d)", MAX_INCLUDE_DEPTH);
        return -1;
    }
    strncpy(pp.include_stack[pp.include_depth], path, MAX_PATH_LEN - 1);
    pp.include_stack[pp.include_depth][MAX_PATH_LEN - 1] = '\0';
    pp.include_depth++;
    return 0;
}

static void pop_include(void) {
    if (pp.include_depth > 0) pp.include_depth--;
}

/* ======================== Procesamiento principal ======================== */

/* Procesa un archivo de forma recursiva. Escribe la salida a 'out'. */
static int process_file(const char *filepath, FILE *out) {
    FILE *in = fopen(filepath, "r");
    if (!in) {
        snprintf(last_error, sizeof(last_error),
                 "No se pudo abrir archivo: %s", filepath);
        return -1;
    }

    if (is_circular(filepath)) {
        snprintf(last_error, sizeof(last_error),
                 "Inclusión circular detectada: %s", filepath);
        fclose(in);
        return -1;
    }

    if (push_include(filepath) != 0) {
        fclose(in);
        return -1;
    }

    char base_dir[MAX_PATH_LEN];
    path_dirname(filepath, base_dir, sizeof(base_dir));

    char line[MAX_LINE];
    char cleaned[MAX_LINE];
    char substituted[MAX_LINE];

    while (fgets(line, sizeof(line), in) != NULL) {
        /* Paso 1: Eliminar comentarios */
        strip_comments(line, cleaned, sizeof(cleaned));

        /* Paso 2: Detectar directivas del preprocesador */
        const char *p = cleaned;
        while (*p && isspace((unsigned char)*p) && *p != '\n') p++;

        if (*p == '#') {
            p++;
            while (*p && isspace((unsigned char)*p)) p++;

            /* === #include "archivo" === */
            if (strncmp(p, "include", 7) == 0 && !is_ident(p[7])) {
                p += 7;
                while (*p && isspace((unsigned char)*p)) p++;

                if (*p == '"') {
                    p++;
                    const char *end = strchr(p, '"');
                    if (end && end > p) {
                        char inc_name[MAX_PATH_LEN];
                        size_t name_len = (size_t)(end - p);
                        if (name_len >= MAX_PATH_LEN) name_len = MAX_PATH_LEN - 1;
                        memcpy(inc_name, p, name_len);
                        inc_name[name_len] = '\0';

                        /* Resolver ruta relativa al archivo actual */
                        char resolved[MAX_PATH_LEN * 2];
                        snprintf(resolved, sizeof(resolved), "%s/%s", base_dir, inc_name);

                        /* Procesar recursivamente */
                        if (process_file(resolved, out) != 0) {
                            fclose(in);
                            pop_include();
                            return -1;
                        }
                        /* Emitir línea vacía para preservar numeración */
                        fprintf(out, "\n");
                        continue;
                    }
                }
                /* #include <...> (sistema) o malformado — emitir línea vacía */
                fprintf(out, "\n");
                continue;
            }
            /* === #define NOMBRE VALOR === */
            else if (strncmp(p, "define", 6) == 0 && !is_ident(p[6])) {
                p += 6;
                while (*p && isspace((unsigned char)*p)) p++;

                /* Extraer nombre */
                const char *name_start = p;
                while (*p && is_ident(*p)) p++;
                size_t name_len = (size_t)(p - name_start);

                if (name_len > 0) {
                    char def_name[MAX_DEFINE_NAME];
                    if (name_len >= MAX_DEFINE_NAME) name_len = MAX_DEFINE_NAME - 1;
                    memcpy(def_name, name_start, name_len);
                    def_name[name_len] = '\0';

                    /* Saltar espacios antes del valor */
                    while (*p && isspace((unsigned char)*p) && *p != '\n') p++;

                    /* Extraer valor (resto de la línea, sin trailing whitespace) */
                    char def_val[MAX_DEFINE_VAL];
                    size_t vi = 0;
                    while (*p && *p != '\n' && vi < MAX_DEFINE_VAL - 1) {
                        def_val[vi++] = *p++;
                    }
                    while (vi > 0 && isspace((unsigned char)def_val[vi - 1])) {
                        vi--;
                    }
                    def_val[vi] = '\0';

                    if (store_define(def_name, def_val) != 0) {
                        fclose(in);
                        pop_include();
                        return -1;
                    }
                }
                /* Emitir línea vacía para preservar numeración */
                fprintf(out, "\n");
                continue;
            }
            /* Otras directivas desconocidas — pasar como texto o emitir vacío */
        }

        /* Paso 3: Aplicar sustituciones de #define */
        apply_defines(cleaned, substituted, sizeof(substituted));

        /* Paso 4: Escribir al archivo de salida */
        fprintf(out, "%s", substituted);
    }

    fclose(in);
    pop_include();
    return 0;
}

/* ======================== API pública ======================== */

int preprocess_file(const char *input_path, const char *output_path) {
    /* Reiniciar estado */
    memset(&pp, 0, sizeof(pp));
    last_error[0] = '\0';

    FILE *out = fopen(output_path, "w");
    if (!out) {
        snprintf(last_error, sizeof(last_error),
                 "Error abriendo archivo de salida: %s", output_path);
        return -1;
    }

    int result = process_file(input_path, out);
    fclose(out);

    /* Verificar comentario de bloque sin cerrar */
    if (result == 0 && pp.in_block_comment) {
        snprintf(last_error, sizeof(last_error),
                 "Comentario de bloque sin cerrar al final del archivo");
        return -1;
    }

    return result;
}

const char *preprocessor_last_error(void) {
    return last_error[0] != '\0' ? last_error : NULL;
}
