#ifndef SCANNER_INTERFACE_H
#define SCANNER_INTERFACE_H

#include "token.h"

//scanner funciones publicas 

// Abre un archivo para escaneo
int scanner_open(const char *path);

// Obtiene el siguiente token
Token get_token(void);

// Cierra el scanner
void scanner_close(void);

// Obtiene el último error
const char *scanner_last_error(void);

#endif 