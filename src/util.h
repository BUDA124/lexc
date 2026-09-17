#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* Crea el directorio si no existe. Retorna 0 en éxito, -1 en error. */
int ensure_directory_exists(const char *dir_path);

/* Genera una ruta única para el archivo temporal preprocesado.
 * Formato: output/preprocessed_<base>_<pid>_<timestamp>.tmp
 * Retorna 0 en éxito, -1 si el buffer es insuficiente. */
int build_temp_path(const char *input_path, char *out_buf, size_t buf_size);

/* Sanitiza un nombre de archivo eliminando caracteres no alfanuméricos */
void sanitize_filename(const char *src, char *dst, size_t max_len);

#endif /* UTIL_H */
