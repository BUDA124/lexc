/* Test: Programa completo en el lenguaje custom */

#define MAX 100
#define MSG "Resultado"

decvar
    integer x;
    integer i;
    float result;
    char c;
endec

x = MAX;
c = 'Z';

for (i = 0; i < MAX; i = i + 1) {
    x = x - 1;
}

if (x == 0) {
    write(MSG);
} elif (x > 0) {
    write("Positivo");
} else {
    write("Negativo");
}

while (x != 0) {
    x = x - 1;
}

read(result);
return 0;
end

