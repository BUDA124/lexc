# ============================================================
# Makefile — Proyecto 1: Analizador Léxico
# ============================================================

CC       := gcc
CFLAGS   := -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -g -Isrc
LDFLAGS  := -lfl

SRC_DIR  := src
BUILD_DIR := build
OUTPUT_DIR := output

TARGET   := analizador

# Objetos (en build/)
OBJS := $(BUILD_DIR)/main.o      \
        $(BUILD_DIR)/cli.o       \
        $(BUILD_DIR)/util.o      \
        $(BUILD_DIR)/stats.o     \
        $(BUILD_DIR)/token.o     \
        $(BUILD_DIR)/preprocessor.o \
        $(BUILD_DIR)/report.o    \
        $(BUILD_DIR)/lex.yy.o

# ============================================================
# Regla principal
# ============================================================

.PHONY: all clean test package

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ============================================================
# Reglas de compilación de objetos
# ============================================================

# Archivos .c normales -> build/*.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Flex: scanner.l -> build/lex.yy.c -> build/lex.yy.o
$(BUILD_DIR)/lex.yy.c: $(SRC_DIR)/scanner.l | $(BUILD_DIR)
	flex -o $@ $<

# Compilar lex.yy.c con -w (suprimir warnings del código generado)
$(BUILD_DIR)/lex.yy.o: $(BUILD_DIR)/lex.yy.c | $(BUILD_DIR)
	$(CC) -std=c11 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -g -Isrc -w -c -o $@ $<

# ============================================================
# Creación de directorios
# ============================================================

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# ============================================================
# Limpieza
# ============================================================

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	rm -rf $(OUTPUT_DIR)/*

# ============================================================
# Pruebas
# ============================================================

test: $(TARGET)
	bash tests/run_tests.sh

# ============================================================
# Empaquetado
# ============================================================

package: clean
	tar -czf Proyecto1_Grupo.tgz \
		$(SRC_DIR)/ \
		tests/ \
		assets/ \
		Makefile \
		README.md \
		.gitignore

