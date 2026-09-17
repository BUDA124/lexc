/* STUB: Implementación provisional del preprocesador (Persona 2 reemplazará este archivo) */
#include "preprocessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char last_error[256] = {0};

int preprocess_file(const char *input_path, const char *output_path) {
    FILE *in = fopen(input_path, "r");
    if (!in) {
        snprintf(last_error, sizeof(last_error), "Error abriendo archivo de entrada: %s", input_path);
        return -1;
    }
    FILE *out = fopen(output_path, "w");
    if (!out) {
        snprintf(last_error, sizeof(last_error), "Error abriendo archivo de salida: %s", output_path);
        fclose(in);
        return -1;
    }

    int c;
    while ((c = fgetc(in)) != EOF) {
        fputc(c, out);
    }

    fclose(in);
    fclose(out);
    return 0;
}

const char *preprocessor_last_error(void) {
    return last_error[0] != '\0' ? last_error : NULL;
}
