# Proyecto 1: Analizador Léxico

Este proyecto implementa un analizador léxico para el lenguaje C, integrando preprocesamiento de directivas y generación automática de un reporte estadístico en PDF utilizando LaTeX/Beamer.

## Requisitos Previos

- Linux (o entorno compatible como WSL)
- GCC (>= 9)
- Flex (>= 2.6)
- GNU Make
- TeX Live con los paquetes `beamer` y `pgfplots`

**Instalación en Ubuntu/Debian:**
```bash
sudo apt install gcc flex make texlive-latex-extra texlive-pictures libfl-dev
```

## Compilación

- `make`: Compila el proyecto completo y genera el ejecutable `analizador`.
- `make clean`: Elimina archivos generados, temporales y objetos (`build/`, `output/*`).
- `make test`: Ejecuta la suite de pruebas automatizadas.
- `make package`: Empaqueta el código fuente limpio en `Proyecto1_Grupo.tgz`.

## Uso

El ejecutable recibe opciones y el archivo de entrada mediante la línea de comandos:

```bash
./analizador [OPCIONES] <archivo_entrada>
```

**Opciones:**
- `-o <ruta>`: Ruta de salida del PDF generado (por defecto: `output/<nombre>.pdf`).
- `-t <ruta>`: Ruta de salida del archivo .tex (por defecto: `output/<nombre>.tex`).
- `-n`: No abrir el visor de PDF al finalizar la ejecución.
- `-h`: Muestra el mensaje de ayuda con la sintaxis y opciones.

**Ejemplos:**
```bash
./analizador tests/input/basic.c
./analizador -n tests/input/basic.c
./analizador -n -o output/reporte.pdf -t output/reporte.tex tests/input/basic.c
./analizador -h
```

## Estructura del Proyecto

```text
.
├── src/                  # Código fuente C y Flex
│   ├── main.c            # Pipeline central y orquestación
│   ├── cli.c / cli.h     # Procesamiento de argumentos
│   ├── util.c / util.h   # Generación de temporales y utilidades
│   ├── token.c / token.h # Estructura y manejo de tokens
│   ├── stats.c / stats.h # Acumulador de estadísticas
│   ├── preprocessor.h    # Contrato del preprocesador
│   ├── scanner_interface.h # Contrato del scanner
│   └── report.h          # Contrato del reporte Beamer
├── assets/               # Recursos de presentación
│   └── beamer_theme.tex  # Tema y paleta de colores para Beamer
├── tests/                # Casos de prueba y script de validación
│   ├── input/            # Archivos de entrada para pruebas
│   ├── expected/         # Salidas esperadas
│   └── run_tests.sh      # Suite de pruebas automatizadas
├── docs/                 # Documentación adicional
├── Makefile              # Reglas de compilación y empaquetado
├── README.md             # Instrucciones y documentación
└── .gitignore            # Exclusión de binarios y temporales
```

## Equipo y Roles

- **Persona 1**: Líder e Integrador (Arquitectura base, CLI, integración y QA)
- **Persona 2**: Preprocesador (Comentarios, directivas `#include` y `#define`)
- **Persona 3**: Scanner Flex (Tokenización y reconocimiento léxico)
- **Persona 4**: Reporte Beamer (Generación LaTeX, diapositivas y gráficas `pgfplots`)

## Códigos de Retorno

- `0`: Ejecución exitosa.
- `1`: Error en argumentos, archivo no encontrado o falla en alguna etapa.
