#include <stdio.h>

int main(void) {
    int x = 42;
    float pi = 3.14;
    char c = 'A';
    char *msg = "Hola mundo";

    if (x > 0) {
        printf("%s: %d\n", msg, x);
    }

    for (int i = 0; i < 10; i++) {
        x += i;
    }

    return 0;
}
