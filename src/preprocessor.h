#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

/*
 * Contrato del módulo de preprocesamiento (Persona 2).
 *
 * Responsabilidades:
 * - Eliminar comentarios de línea (//) y comentarios de bloque
 * - Expandir directivas #include (archivos locales, cualquier extensión)
 * - Expandir #define simples (sustitución textual sin parámetros)
 * - Conservar la estructura de líneas para que el scanner reporte
 *   posiciones correctas
 */

/* Preprocesa el archivo de entrada y escribe el resultado en output_path.
 * Retorna 0 en éxito, -1 en error. */
int preprocess_file(const char *input_path, const char *output_path);

/* Retorna un mensaje descriptivo del último error ocurrido, o NULL. */
const char *preprocessor_last_error(void);

#endif /* PREPROCESSOR_H */
