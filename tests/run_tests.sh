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

# ============================================================
# Helper: prueba de ejecución (verifica exit code)
# ============================================================
run_test() {
    local test_name="$1"
    local input_file="$2"
    local expect_success="$3"  # 1 = expect exit 0, 0 = expect exit != 0
    local extra_flags="$4"     # flags adicionales (opcional)

    echo -n "  Test: $test_name ... "

    if $BINARY -s $extra_flags "$input_file" > /dev/null 2>&1; then
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

echo "========================================================"
echo " Suite de Pruebas - Analizador Léxico"
echo "========================================================"
echo ""

# ============================================================
# 1. Pruebas de escaneo exitoso (modo -s: sin generar reporte)
# ============================================================
echo "--- Pruebas de escaneo exitoso (modo scan-only) ---"
run_test "Archivo básico C"               tests/input/basic.c         1
run_test "Archivo con comentarios"         tests/input/comments.c      1
run_test "Archivo con defines"             tests/input/defines.c       1
run_test "Archivo con include local"       tests/input/include_test.c  1
run_test "Archivo con errores léxicos"     tests/input/errors.c        1
run_test "Archivo vacío"                   tests/input/empty.c         1
run_test "Literales diversos"              tests/input/literals.c      1
run_test "Archivo con includes (includes.c)"  tests/input/includes.c      1
run_test "Todos los operadores"            tests/input/operators.c     1
run_test "Lenguaje custom completo"        tests/input/custom_lang.c   1
run_test "Sin newline al final"            tests/input/no_newline.c    1
run_test "Entrada binaria"                 tests/input/binary_input.bin 1

# ============================================================
# 2. Pruebas de preprocesador — fallos controlados
# ============================================================
echo ""
echo "--- Pruebas de fallo controlado (preprocesador) ---"
run_test "Archivo inexistente"             no_existe.xyz               0
run_test "Comentario de bloque sin cerrar" tests/input/comment_unclosed.c 0
run_test "Include de archivo faltante"     tests/input/include_missing.c  0
run_test "Inclusión circular"              tests/input/circular_a.c       0

# ============================================================
# 3. Pruebas de CLI
# ============================================================
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

echo -n "  Test: Flag -s (scan-only) ... "
if $BINARY -s tests/input/basic.c > /dev/null 2>&1; then
    echo "PASS ✓"
    PASS=$((PASS + 1))
else
    echo "FAIL ✗"
    FAIL=$((FAIL + 1))
fi

# ============================================================
# 4. Verificación del preprocesador
# ============================================================
echo ""
echo "--- Verificación del preprocesador ---"

# Test: comentarios eliminados correctamente
echo -n "  Test: Comentarios eliminados ... "
$BINARY -s tests/input/comments.c > /dev/null 2>&1
TEMP_FILE=$(ls -t output/preprocessed_comments_c_*.tmp 2>/dev/null | head -1)
if [ -n "$TEMP_FILE" ]; then
    # Verificar que los comentarios reales fueron eliminados
    if grep -q 'Este es un comentario' "$TEMP_FILE" 2>/dev/null; then
        echo "FAIL ✗ (comentario de línea no eliminado)"
        FAIL=$((FAIL + 1))
    elif grep -q 'comentario al final' "$TEMP_FILE" 2>/dev/null; then
        echo "FAIL ✗ (comentario al final de línea no eliminado)"
        FAIL=$((FAIL + 1))
    elif grep -q 'multilínea' "$TEMP_FILE" 2>/dev/null; then
        echo "FAIL ✗ (comentario de bloque no eliminado)"
        FAIL=$((FAIL + 1))
    elif ! grep -q 'esto no es // un comentario' "$TEMP_FILE" 2>/dev/null; then
        echo "FAIL ✗ (secuencia // dentro de string fue eliminada por error)"
        FAIL=$((FAIL + 1))
    elif ! grep -q 'esto tampoco /\* es \*/ un comentario' "$TEMP_FILE" 2>/dev/null; then
        echo "FAIL ✗ (secuencia /* */ dentro de string fue eliminada por error)"
        FAIL=$((FAIL + 1))
    else
        echo "PASS ✓"
        PASS=$((PASS + 1))
    fi
else
    echo "FAIL ✗ (archivo temporal no encontrado)"
    FAIL=$((FAIL + 1))
fi

# Test: #define expandido correctamente
echo -n "  Test: Defines expandidos ... "
$BINARY -s tests/input/defines.c > /dev/null 2>&1
TEMP_FILE=$(ls -t output/preprocessed_defines_c_*.tmp 2>/dev/null | head -1)
if [ -n "$TEMP_FILE" ]; then
    # Después de preprocesar, las líneas con #define deben desaparecer
    # y las referencias a MAX, MIN, MSG deben estar sustituidas
    if grep -q '#define' "$TEMP_FILE" 2>/dev/null; then
        echo "FAIL ✗ (directivas #define no removidas)"
        FAIL=$((FAIL + 1))
    elif grep -q '\bMAX\b' "$TEMP_FILE" 2>/dev/null; then
        echo "FAIL ✗ (macro MAX no expandida)"
        FAIL=$((FAIL + 1))
    else
        echo "PASS ✓"
        PASS=$((PASS + 1))
    fi
else
    echo "FAIL ✗ (archivo temporal no encontrado)"
    FAIL=$((FAIL + 1))
fi

# Test: #include expandido correctamente
echo -n "  Test: Include local expandido ... "
$BINARY -s tests/input/include_test.c > /dev/null 2>&1
TEMP_FILE=$(ls -t output/preprocessed_include_test_c_*.tmp 2>/dev/null | head -1)
if [ -n "$TEMP_FILE" ]; then
    # Después de preprocesar, el contenido de header.txt debe estar incluido
    # y MAGIC_NUMBER (definido en header.txt) debe estar expandido
    if grep -q '#include' "$TEMP_FILE" 2>/dev/null; then
        echo "FAIL ✗ (directiva #include no removida)"
        FAIL=$((FAIL + 1))
    elif grep -q 'helper_var' "$TEMP_FILE" 2>/dev/null; then
        echo "PASS ✓"
        PASS=$((PASS + 1))
    else
        echo "FAIL ✗ (contenido del include no encontrado)"
        FAIL=$((FAIL + 1))
    fi
else
    echo "FAIL ✗ (archivo temporal no encontrado)"
    FAIL=$((FAIL + 1))
fi

# ============================================================
# 5. Verificación de archivos generados
# ============================================================
echo ""
echo "--- Verificación de archivos generados ---"
echo -n "  Test: Archivos temporales creados ... "
TEMP_COUNT=$(ls output/preprocessed_*.tmp 2>/dev/null | wc -l)
if [ "$TEMP_COUNT" -gt 0 ]; then
    echo "PASS ✓ ($TEMP_COUNT archivo(s) temporal(es))"
    PASS=$((PASS + 1))
else
    echo "FAIL ✗ (no se encontraron archivos temporales)"
    FAIL=$((FAIL + 1))
fi

# ============================================================
# Resultados
# ============================================================
echo ""
echo "========================================================"
echo " Resultados: $PASS pasados, $FAIL fallidos"
echo "========================================================"

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
