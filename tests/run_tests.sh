#!/bin/bash
# Suite de pruebas automatizadas - Proyecto 1: Analizador Léxico

set -e

PASS=0
FAIL=0
BINARY="./analizador"

if [ ! -f "$BINARY" ]; then
    echo "ERROR: No se encontró el binario '$BINARY'. Ejecute 'make' primero."
    exit 1
fi

run_test() {
    local test_name="$1"
    local input_file="$2"
    local expect_success="$3"  # 1 = expect exit 0, 0 = expect exit != 0

    echo -n "  Test: $test_name ... "

    if $BINARY -n "$input_file" > /dev/null 2>&1; then
        actual_exit=0
    else
        actual_exit=1
    fi

    if [ "$expect_success" -eq 1 ] && [ "$actual_exit" -eq 0 ]; then
        echo "PASS ✓"
        PASS=$((PASS + 1))
    elif [ "$expect_success" -eq 0 ] && [ "$actual_exit" -ne 0 ]; then
        echo "PASS ✓ (fallo esperado)"
        PASS=$((PASS + 1))
    else
        echo "FAIL ✗ (exit=$actual_exit, esperado=$([ $expect_success -eq 1 ] && echo '0' || echo '!=0'))"
        FAIL=$((FAIL + 1))
    fi
}

echo "========================================"
echo " Suite de Pruebas - Analizador Léxico"
echo "========================================"
echo ""

echo "--- Pruebas de ejecución exitosa ---"
run_test "Archivo básico C"               tests/input/basic.c        1
run_test "Archivo con comentarios"         tests/input/comments.c     1
run_test "Archivo con defines"             tests/input/defines.c      1
run_test "Archivo con include"             tests/input/include_test.c 1
run_test "Archivo con errores léxicos"     tests/input/errors.c       1
run_test "Archivo vacío"                   tests/input/empty.c        1

echo ""
echo "--- Pruebas de fallo controlado ---"
run_test "Archivo inexistente"             no_existe.xyz              0

echo ""
echo "--- Pruebas de CLI ---"
echo -n "  Test: Flag -h (ayuda) ... "
if $BINARY -h > /dev/null 2>&1; then
    echo "PASS ✓"
    PASS=$((PASS + 1))
else
    echo "FAIL ✗"
    FAIL=$((FAIL + 1))
fi

echo -n "  Test: Sin argumentos ... "
if $BINARY > /dev/null 2>&1; then
    echo "FAIL ✗ (debería fallar sin argumentos)"
    FAIL=$((FAIL + 1))
else
    echo "PASS ✓ (fallo esperado)"
    PASS=$((PASS + 1))
fi

# Verify temp file creation
echo ""
echo "--- Verificación de archivos generados ---"
echo -n "  Test: Archivo temporal creado ... "
TEMP_COUNT=$(ls output/preprocessed_*.tmp 2>/dev/null | wc -l)
if [ "$TEMP_COUNT" -gt 0 ]; then
    echo "PASS ✓ ($TEMP_COUNT archivo(s) temporal(es))"
    PASS=$((PASS + 1))
else
    echo "FAIL ✗ (no se encontraron archivos temporales)"
    FAIL=$((FAIL + 1))
fi

echo -n "  Test: Archivo .tex generado ... "
TEX_COUNT=$(ls output/*.tex 2>/dev/null | wc -l)
if [ "$TEX_COUNT" -gt 0 ]; then
    echo "PASS ✓ ($TEX_COUNT archivo(s) .tex)"
    PASS=$((PASS + 1))
else
    echo "FAIL ✗ (no se encontraron archivos .tex)"
    FAIL=$((FAIL + 1))
fi

echo -n "  Test: Archivo .pdf generado ... "
PDF_COUNT=$(ls output/*.pdf 2>/dev/null | wc -l)
if [ "$PDF_COUNT" -gt 0 ]; then
    echo "PASS ✓ ($PDF_COUNT archivo(s) .pdf)"
    PASS=$((PASS + 1))
else
    echo "FAIL ✗ (no se encontraron archivos .pdf)"
    FAIL=$((FAIL + 1))
fi

echo ""
echo "========================================"
echo " Resultados: $PASS pasados, $FAIL fallidos"
echo "========================================"

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
