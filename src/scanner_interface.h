#ifndef SCANNER_INTERFACE_H
#define SCANNER_INTERFACE_H

#include "token.h"

/*
 * Contrato del módulo del scanner léxico (Persona 3).
 *
 * Responsabilidades:
 * - Abrir un archivo preprocesado y tokenizarlo secuencialmente
 * - Reconocer todas las categorías definidas en TokenType
 * - Reportar errores léxicos como TOK_LEXICAL_ERROR
 * - Rastrear línea y columna de cada token
 */

/* Abre el archivo para escaneo. Retorna 0 en éxito, -1 en error. */
int scanner_open(const char *path);

/* Extrae y retorna el siguiente token. Retorna TOK_EOF al final. */
Token get_token(void);

/* Cierra el archivo abierto y libera recursos del scanner. */
void scanner_close(void);

/* Retorna un mensaje descriptivo del último error, o NULL. */
const char *scanner_last_error(void);

#endif /* SCANNER_INTERFACE_H */
