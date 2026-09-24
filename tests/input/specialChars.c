/* Caracteres que rompen LaTeX si no se escapan */
int main(void) {
    char *s = "\\ { } _ # % & $ ^ ~ < > | [ ] -- ,, `` '' !` ?`";
    int a_b = 5 % 3;
    int c = a_b & 1 | 2 ^ 3;
    unsigned mask = ~0;
    char *acentos = "canción ñandú ¿qué? ¡sí! €100 ";
    c = c && c || !c;
    return a_b >= 10 ? 1 : 0;
}
