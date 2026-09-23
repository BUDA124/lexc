#include "report.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/*Parámetros editables*/

#define REPORT_TITLE          "Analizador léxico"
#define REPORT_SUBTITLE       "Proyecto 1: preproceso y scanner con Flex"
#define REPORT_COURSE         "Compiladores e Intérpretes"
#define REPORT_INSTITUTION    "Instituto Tecnológico de Costa Rica"
#define THEME_RELATIVE_PATH   "assets/beamer_theme.tex"

#define CODE_ROWS_PER_FRAME   16   /* filas visibles por diapositiva de código */
#define CODE_MAX_COLS         92   /* columnas antes de partir visualmente   */
#define CODE_CONT_INDENT      2    /* sangría de las filas de continuación    */
#define CODE_MAX_BLANK_RUN    2    /* líneas vacías seguidas que se muestran  */
#define ERR_ROWS_PER_FRAME    10
#define ERR_MAX_LISTED        40
#define DETAIL_MAX_TYPES      16
#define PDFLATEX_TIMEOUT_S    240


/* Manejo de errores */

static char g_error[1024];

static void set_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_error, sizeof g_error, fmt, ap);
    va_end(ap);
}

const char *report_last_error(void)
{
    return g_error[0] ? g_error : NULL;
}


/* Buffer de texto dinámico */

typedef struct {
    char  *data;
    size_t len;
    size_t cap;
    int    oom;
} Buf;

static int buf_reserve(Buf *b, size_t extra)
{
    if (b->oom) return -1;
    if (b->len + extra + 1 <= b->cap) return 0;
    size_t ncap = b->cap ? b->cap : 64;
    while (ncap < b->len + extra + 1) ncap *= 2;
    char *p = realloc(b->data, ncap);
    if (!p) { b->oom = 1; return -1; }
    b->data = p;
    b->cap = ncap;
    return 0;
}

static void buf_putn(Buf *b, const char *s, size_t n)
{
    if (buf_reserve(b, n) != 0) return;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void buf_puts(Buf *b, const char *s) { buf_putn(b, s, strlen(s)); }

static void buf_printf(Buf *b, const char *fmt, ...)
{
    char tmp[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n < sizeof tmp) { buf_putn(b, tmp, (size_t)n); return; }
    if (buf_reserve(b, (size_t)n) != 0) return;
    va_start(ap, fmt);
    vsnprintf(b->data + b->len, (size_t)n + 1, fmt, ap);
    va_end(ap);
    b->len += (size_t)n;
}

static const char *buf_str(const Buf *b) { return b->data ? b->data : ""; }
static void buf_free(Buf *b) { free(b->data); b->data = NULL; b->len = b->cap = 0; }


/* Categorías léxicas del reporte */

typedef enum {
    CAT_KEYWORD, CAT_IDENT, CAT_INT, CAT_FLOAT, CAT_CHAR,
    CAT_STRING, CAT_OP, CAT_SEP, CAT_ERROR,
    CAT_COUNT,
    CAT_NONE      /* espacios, saltos de línea y EOF: no se grafican */
} Category;

typedef struct {
    const char *plural;
    const char *singular;
    const char *macro;
    const char *color;
    const char *style;
} CatInfo;

static const CatInfo CATS[CAT_COUNT] = {
    { "Palabras clave",  "Palabra clave",       "tokenKeyword",   "TokKeyword", "Azul, negrita" },
    { "Identificadores", "Identificador",       "tokenIdent",     "TokIdent",   "Casi negro, regular" },
    { "Enteros",         "Literal entero",      "tokenInteger",   "TokInt",     "Naranja, oblicua" },
    { "Flotantes",       "Literal flotante",    "tokenFloat",     "TokFloat",   "Café, negrita oblicua" },
    { "Caracteres",      "Literal de carácter", "tokenChar",      "TokChar",    "Verde azulado, itálica, fondo celeste" },
    { "Cadenas",         "Literal de cadena",   "tokenString",    "TokString",  "Verde, fondo verde claro" },
    { "Operadores",      "Operador",            "tokenOperator",  "TokOp",      "Morado, negrita" },
    { "Separadores",     "Separador",           "tokenSeparator", "TokSep",     "Gris, regular" },
    { "Errores léxicos", "Error léxico",        "tokenError",     "TokErr",     "Rojo, negrita, fondo rosa" },
};

/*
 * Es es el ÚNICO punto del módulo que depende de los valores concretos deTokenType. 
 * (TOK_KEYWORD, TOK_IDENTIFIER, TOK_INTEGER_LITERAL, ...):
 * 
 *     case TOK_KEYWORD:         return CAT_KEYWORD;
 *     case TOK_IDENTIFIER:      return CAT_IDENT;
 *     case TOK_INTEGER_LITERAL: return CAT_INT;
 *     case TOK_FLOAT_LITERAL:   return CAT_FLOAT;
 *     case TOK_CHAR_LITERAL:    return CAT_CHAR;
 *     case TOK_STRING_LITERAL:  return CAT_STRING;
 *     case TOK_OPERATOR:        return CAT_OP;
 *     case TOK_SEPARATOR:       return CAT_SEP;
 *     case TOK_LEXICAL_ERROR:   return CAT_ERROR;
 *     default:                  return CAT_NONE;
 */
static Category classify(TokenType t)
{
    int v = (int)t;
    switch (t) {
    case TOK_IDENTIFICADOR: return CAT_IDENT;
    case TOK_CONSTAINTEGER: return CAT_INT;
    case TOK_CONSTAFLOAT:   return CAT_FLOAT;
    case TOK_CONSTCADENA:   return CAT_STRING;
    case TOK_LEXICAL_ERROR: return CAT_ERROR;
    case TOK_WHITESPACE:
    case TOK_NEWLINE:
    case TOK_EOF:           return CAT_NONE;
    default:                break;
    }
    if (v >= (int)TOK_IF && v <= (int)TOK_READ)                   return CAT_KEYWORD;
    if (v >= (int)TOK_OPSUMA && v <= (int)TOK_OPDOSPUNTOS)        return CAT_OP;
    if (v >= (int)TOK_LLAVEABIERTA && v <= (int)TOK_CHARPUNTO)    return CAT_SEP;
    return CAT_NONE;
}


/* Escape de LaTeX */

/*
Con esto ningún byte de la entrada puede romper
pdflatex ni desalinear el texto monoespaciado. */


typedef struct {
    size_t nbytes;
    int    width;
    int    newline;
    char   tex[40];
} Unit;

static int utf8_printable(unsigned long cp)
{
    if (cp >= 0xC0 && cp <= 0xFF && cp != 0xD7 && cp != 0xF7) return 1;
    switch (cp) {
    case 0xA1: case 0xBF: case 0xAB: case 0xBB:
    case 0x2013: case 0x2014: case 0x2018: case 0x2019: case 0x201C: case 0x201D:
        return 1;
    default:
        return 0;
    }
}

static const char *ascii_escape(unsigned char c)
{
    switch (c) {
    case '\\': return "\\textbackslash{}";
    case '{':  return "\\{";
    case '}':  return "\\}";
    case '_':  return "\\_";
    case '#':  return "\\#";
    case '%':  return "\\%";
    case '&':  return "\\&";
    case '$':  return "\\$";
    case '^':  return "\\textasciicircum{}";
    case '~':  return "\\textasciitilde{}";
    case '<':  return "\\textless{}";
    case '>':  return "\\textgreater{}";
    case '|':  return "\\textbar{}";
    case '[':  return "{[}";
    case ']':  return "{]}";

    case '-':  return "-{}";
    case ',':  return ",{}";
    case '\'': return "'{}";
    case '`':  return "`{}";
    case '"':  return "\"{}";
    case '!':  return "!{}";
    case '?':  return "?{}";
    default:   return NULL;
    }
}

static void decode_unit(const unsigned char *s, size_t n, int code_mode, Unit *u)
{
    unsigned char c = s[0];
    u->nbytes = 1;
    u->width = 1;
    u->newline = 0;
    u->tex[0] = '\0';

    if (c == '\n') { u->newline = 1; u->width = 0; return; }
    if (c == ' ' || c == '\t') { strcpy(u->tex, code_mode ? "~" : " "); return; }
    if (c < 0x20 || c == 0x7F) {
        snprintf(u->tex, sizeof u->tex, "\\textbackslash{}x%02X", (unsigned)c);
        u->width = 4;
        return;
    }
    if (c < 0x80) {
        const char *e = ascii_escape(c);
        if (e) strcpy(u->tex, e);
        else { u->tex[0] = (char)c; u->tex[1] = '\0'; }
        return;
    }

    size_t len = 0;
    unsigned long cp = 0;
    if (c >= 0xC2 && c <= 0xDF)      { len = 2; cp = c & 0x1Fu; }
    else if (c >= 0xE0 && c <= 0xEF) { len = 3; cp = c & 0x0Fu; }
    else if (c >= 0xF0 && c <= 0xF4) { len = 4; cp = c & 0x07u; }

    if (len > 0 && len <= n) {
        size_t i;
        for (i = 1; i < len; i++) {
            if ((s[i] & 0xC0u) != 0x80u) break;
            cp = (cp << 6) | (s[i] & 0x3Fu);
        }
        int valid = (i == len)
                 && !(len == 3 && cp < 0x800)
                 && !(len == 4 && (cp < 0x10000 || cp > 0x10FFFF))
                 && !(cp >= 0xD800 && cp <= 0xDFFF);
        if (valid) {
            u->nbytes = len;
            if (utf8_printable(cp)) {
                memcpy(u->tex, s, len);
                u->tex[len] = '\0';
                u->width = 1;
            } else {
                snprintf(u->tex, sizeof u->tex, "\\textbackslash{}u%04lX", cp);
                u->width = 2 + (cp > 0xFFFFF ? 6 : cp > 0xFFFF ? 5 : 4);
            }
            return;
        }
    }
    snprintf(u->tex, sizeof u->tex, "\\textbackslash{}x%02X", (unsigned)c);
    u->width = 4;
}

/* Escapa texto para celdas, títulos y portada. Los saltos de línea se
 * muestran como \n. max_width = 0 , sin un límite*/

static int escape_into(Buf *b, const char *s, int code_mode, int max_width)
{
    const unsigned char *p = (const unsigned char *)(s ? s : "");
    size_t n = strlen((const char *)p);
    int width = 0;
    size_t i = 0;
    while (i < n) {
        Unit u;
        decode_unit(p + i, n - i, code_mode, &u);
        const char *tex = u.newline ? "\\textbackslash{}n" : u.tex;
        int w = u.newline ? 2 : u.width;
        if (max_width > 0 && width + w > max_width) {
            buf_puts(b, "\\ldots{}");
            return width + 1;
        }
        buf_puts(b, tex);
        width += w;
        i += u.nbytes;
    }
    return width;
}

static void put_escaped(FILE *f, const char *s, int code_mode, int max_width)
{
    Buf b = {0};
    escape_into(&b, s, code_mode, max_width);
    fputs(buf_str(&b), f);
    buf_free(&b);
}

/* Lexema de un token como texto de celda; un error vacío es un byte NUL. */

static void put_lexeme(FILE *f, const Token *t, int max_width)
{
    if (t->lexeme && t->lexeme[0]) put_escaped(f, t->lexeme, 1, max_width);
    else if (t->type == TOK_LEXICAL_ERROR) fputs("\\textbackslash{}x00", f);
}


/* Trigonometría mínima (evita depender de -lm, que el Makefile no usa) */

#define REPORT_PI 3.14159265358979323846

static double r_sin(double x)
{
    while (x > REPORT_PI)  x -= 2.0 * REPORT_PI;
    while (x < -REPORT_PI) x += 2.0 * REPORT_PI;
    double term = x, sum = x, x2 = x * x;
    for (int k = 1; k < 13; k++) {
        term *= -x2 / (double)((2 * k) * (2 * k + 1));
        sum += term;
    }
    return sum;
}

static double r_cos(double x) { return r_sin(x + REPORT_PI / 2.0); }


/* Maquetación de la fuente coloreada                                  */


typedef enum { ROW_LINE, ROW_CONT, ROW_SKIP } RowKind;

typedef struct {
    RowKind kind;
    long    number;  /* línea fuente (ROW_LINE) o líneas omitidas (ROW_SKIP) */
    Buf     text;
} Row;

typedef struct {
    Row   *rows;
    size_t count;
    size_t cap;
    long   src_line;    /* línea fuente de la fila abierta */
    long   src_col;   /* siguiente columna fuente (bytes, base 1) */
    int    vis;     /* columna visual usada en la fila abierta*/
    long   max_line;
    int    oom;
} Layout;

static Row *layout_open_row(Layout *L, RowKind kind, long number)
{
    if (L->oom) return NULL;
    if (L->count == L->cap) {
        size_t ncap = L->cap ? L->cap * 2 : 128;
        Row *p = realloc(L->rows, ncap * sizeof *p);
        if (!p) { L->oom = 1; return NULL; }
        L->rows = p;
        L->cap = ncap;
    }
    Row *r = &L->rows[L->count++];
    memset(r, 0, sizeof *r);
    r->kind = kind;
    r->number = number;
    L->vis = 0;
    if (kind == ROW_CONT) {
        buf_printf(&r->text, "\\cgap{%d}", CODE_CONT_INDENT);
        L->vis = CODE_CONT_INDENT;
    }
    if (kind == ROW_LINE && number > L->max_line) L->max_line = number;
    return r;
}

static Row *layout_current(Layout *L) { return L->count ? &L->rows[L->count - 1] : NULL; }

/* Abre la fila de la línea target, insertando las líneas vacías */


static void layout_goto_line(Layout *L, long target)
{
    long first_blank = L->src_line + 1;
    long blanks = target - first_blank;
    if (blanks > CODE_MAX_BLANK_RUN) {
        layout_open_row(L, ROW_SKIP, blanks);
    } else {
        for (long ln = first_blank; ln < target; ln++) layout_open_row(L, ROW_LINE, ln);
    }
    layout_open_row(L, ROW_LINE, target);
    L->src_line = target;
    L->src_col = 1;
}

static int unit_width_of(const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    size_t n = strlen(s), i = 0;
    int w = 0;
    while (i < n) {
        Unit u;
        decode_unit(p + i, n - i, 1, &u);
        if (u.newline) break;
        w += u.width;
        i += u.nbytes;
    }
    return w;
}

static void layout_token(Layout *L, const Token *t, Category cat)
{
    const char *lex = (t->lexeme && t->lexeme[0]) ? t->lexeme
                    : (cat == CAT_ERROR ? "\x01" : "");
    int nul_marker = !(t->lexeme && t->lexeme[0]) && cat == CAT_ERROR;
    if (!lex[0]) return;

    long line = t->line > 0 ? t->line : (L->src_line > 0 ? L->src_line : 1);
    if (L->count == 0 || line > L->src_line) layout_goto_line(L, line);

    long gap = (t->column > L->src_col) ? t->column - L->src_col : 0;
    if (t->column > L->src_col) L->src_col = t->column;

    int w = nul_marker ? 4 : unit_width_of(lex);
    if ((L->vis > CODE_CONT_INDENT && L->vis + gap + w > CODE_MAX_COLS) ||
        L->vis + gap > CODE_MAX_COLS) {
        layout_open_row(L, ROW_CONT, 0);
        gap = 0;
    }
    Row *r = layout_current(L);
    if (!r) return;
    if (gap > 0) {
        buf_printf(&r->text, "\\cgap{%ld}", gap);
        L->vis += (int)gap;
    }

    const char *macro = CATS[cat].macro;
    int chunk_open = 0;
    const unsigned char *p = (const unsigned char *)lex;
    size_t n = strlen(lex), i = 0;

    while (i < n) {
        Unit u;
        if (nul_marker) {
            strcpy(u.tex, "\\textbackslash{}x00");
            u.width = 4; u.nbytes = n; u.newline = 0;
        } else {
            decode_unit(p + i, n - i, 1, &u);
        }
        if (u.newline) {
            if (chunk_open) { buf_puts(&r->text, "}"); chunk_open = 0; }
            L->src_line++;
            r = layout_open_row(L, ROW_LINE, L->src_line);
            if (!r) return;
            L->src_col = 1;
            i += u.nbytes;
            continue;
        }
        if (L->vis + u.width > CODE_MAX_COLS && L->vis > CODE_CONT_INDENT) {
            if (chunk_open) { buf_puts(&r->text, "}"); chunk_open = 0; }
            r = layout_open_row(L, ROW_CONT, 0);
            if (!r) return;
        }
        if (!chunk_open) { buf_printf(&r->text, "\\%s{", macro); chunk_open = 1; }
        buf_puts(&r->text, u.tex);
        L->vis += u.width;
        L->src_col += (long)u.nbytes;
        i += u.nbytes;
    }
    if (chunk_open) buf_puts(&r->text, "}");
}

static int layout_build(Layout *L, const Token *tokens, size_t count)
{
    memset(L, 0, sizeof *L);
    for (size_t i = 0; i < count; i++) {
        Category c = classify(tokens[i].type);
        if (c == CAT_NONE) continue;
        layout_token(L, &tokens[i], c);
        if (L->oom) return -1;
    }
    for (size_t i = 0; i < L->count; i++) if (L->rows[i].text.oom) return -1;
    return 0;
}

static void layout_free(Layout *L)
{
    for (size_t i = 0; i < L->count; i++) buf_free(&L->rows[i].text);
    free(L->rows);
    memset(L, 0, sizeof *L);
}


/* Datos agregados para el reporte */


typedef struct {
    unsigned long cat_count[CAT_COUNT];
    unsigned long total;  /* suma de categorías graficadas */
    long          first_tok[CAT_COUNT];
    unsigned long error_count;
    long          source_lines;    /* -1 si no se pudo leer el temporal */
    const char   *source_name;
} ReportData;

static long count_file_lines(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    long lines = 0;
    int c, last = '\n', any = 0;
    while ((c = fgetc(f)) != EOF) {
        any = 1;
        if (c == '\n') lines++;
        last = c;
    }
    fclose(f);
    if (any && last != '\n') lines++;
    return lines;
}

static const char *path_basename(const char *p)
{
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

static void collect_data(ReportData *d, const ReportConfig *cfg, const TokenStats *stats)
{
    memset(d, 0, sizeof *d);
    for (int c = 0; c < CAT_COUNT; c++) d->first_tok[c] = -1;

    for (int t = 0; t < (int)TOK_TYPE_COUNT; t++) {
        Category c = classify((TokenType)t);
        if (c == CAT_NONE) continue;
        d->cat_count[c] += stats->counts[t];
        d->total += stats->counts[t];
    }
    d->error_count = d->cat_count[CAT_ERROR];

    for (size_t i = 0; i < cfg->token_count; i++) {
        Category c = classify(cfg->tokens[i].type);
        if (c != CAT_NONE && d->first_tok[c] < 0) d->first_tok[c] = (long)i;
    }
    d->source_lines = count_file_lines(cfg->processed_source_path);
    d->source_name = path_basename(cfg->processed_source_path);
}

static double pct(unsigned long part, unsigned long total)
{
    return total ? 100.0 * (double)part / (double)total : 0.0;
}


/*se incrusta assets/beamer_theme.tex  */


static const char *FALLBACK_THEME =
    "% Tema mínimo embebido (no se encontró " THEME_RELATIVE_PATH ")\n"
    "\\definecolor{Navy}{HTML}{14213D}\\definecolor{NavyLight}{HTML}{25375C}\n"
    "\\definecolor{Accent}{HTML}{F2A541}\\definecolor{Teal}{HTML}{2A9D8F}\n"
    "\\definecolor{Ink}{HTML}{1F2430}\\definecolor{Muted}{HTML}{6B7280}\n"
    "\\definecolor{CodeBg}{HTML}{F7F8FA}\\definecolor{CodeRule}{HTML}{D6DAE1}\n"
    "\\definecolor{LineNo}{HTML}{9AA3AF}\n"
    "\\definecolor{TokKeyword}{HTML}{1D3FA6}\\definecolor{TokIdent}{HTML}{23262E}\n"
    "\\definecolor{TokInt}{HTML}{D2691E}\\definecolor{TokFloat}{HTML}{8F4A0C}\n"
    "\\definecolor{TokString}{HTML}{1E7B34}\\definecolor{TokStringBg}{HTML}{E3F4E7}\n"
    "\\definecolor{TokChar}{HTML}{0B7285}\\definecolor{TokCharBg}{HTML}{DDF3F6}\n"
    "\\definecolor{TokOp}{HTML}{7B2CBF}\\definecolor{TokSep}{HTML}{7A8190}\n"
    "\\definecolor{TokErr}{HTML}{B00020}\\definecolor{TokErrBg}{HTML}{FFD3DA}\n"
    "\\newcommand{\\tokbg}[2]{{\\setlength{\\fboxsep}{0pt}\\colorbox{#1}{\\rule[-0.3em]{0pt}{1.12em}#2}}}\n"
    "\\newcommand{\\tokenKeyword}[1]{{\\bfseries\\color{TokKeyword}#1}}\n"
    "\\newcommand{\\tokenIdent}[1]{{\\color{TokIdent}#1}}\n"
    "\\newcommand{\\tokenInteger}[1]{{\\slshape\\color{TokInt}#1}}\n"
    "\\newcommand{\\tokenFloat}[1]{{\\bfseries\\slshape\\color{TokFloat}#1}}\n"
    "\\newcommand{\\tokenString}[1]{\\tokbg{TokStringBg}{\\color{TokString}#1}}\n"
    "\\newcommand{\\tokenChar}[1]{\\tokbg{TokCharBg}{\\itshape\\color{TokChar}#1}}\n"
    "\\newcommand{\\tokenOperator}[1]{{\\bfseries\\color{TokOp}#1}}\n"
    "\\newcommand{\\tokenSeparator}[1]{{\\color{TokSep}#1}}\n"
    "\\newcommand{\\tokenError}[1]{\\tokbg{TokErrBg}{\\bfseries\\color{TokErr}#1}}\n"
    "\\newlength{\\tokcw}\\newlength{\\codegutterw}\\newsavebox{\\codebox}\n"
    "\\newcommand{\\cgap}[1]{\\hspace*{#1\\tokcw}}\n"
    "\\newcommand{\\codegutter}[1]{\\makebox[\\codegutterw][r]{\\tiny\\color{LineNo}#1}\\hspace{1em}}\n"
    "\\newcommand{\\codeline}[2]{\\codegutter{#1}\\strut#2\\par}\n"
    "\\newcommand{\\codecont}[1]{\\codegutter{$\\hookrightarrow$}\\strut#1\\par}\n"
    "\\newcommand{\\codeskip}[1]{\\codegutter{$\\cdots$}\\strut{\\tiny #1 líneas vacías omitidas}\\par}\n"
    "\\newenvironment{codeblock}[1]{\\begin{lrbox}{\\codebox}\\begin{minipage}[t]{\\dimexpr\\linewidth-12pt\\relax}"
    "\\ttfamily\\scriptsize\\settowidth{\\tokcw}{x}\\settowidth{\\codegutterw}{\\tiny #1}"
    "\\setlength{\\parindent}{0pt}\\setlength{\\baselineskip}{1.16em}}"
    "{\\end{minipage}\\end{lrbox}{\\setlength{\\fboxsep}{5.5pt}\\noindent\\fcolorbox{CodeRule}{CodeBg}{\\usebox{\\codebox}}}}\n"
    "\\newcommand{\\codelegend}{\\par\\vfill}\n"
    "\\setbeamertemplate{navigation symbols}{}\n"
    "\\setbeamercolor{frametitle}{fg=white,bg=Navy}\\setbeamercolor{structure}{fg=Navy}\n"
    "\\setbeamertemplate{footline}[frame number]\n"
    "\\tikzset{pill/.style={rounded corners=2.5pt,draw=#1,fill=white,inner sep=3.5pt,font=\\ttfamily\\footnotesize},"
    "stage/.style={rounded corners=3pt,fill=#1,text=white,align=center,font=\\scriptsize\\bfseries,inner sep=4pt,minimum height=2.5em},"
    "flow/.style={-{Stealth[length=5pt]},line width=0.9pt,draw=Muted},lbl/.style={font=\\tiny,text=Muted,inner sep=1pt},"
    "card/.style={rounded corners=4pt,fill=CodeBg,draw=CodeRule,minimum width=3.2cm,minimum height=2.2cm,align=center}}\n";

static int copy_file_into(FILE *out, const char *path)
{
    FILE *in = fopen(path, "rb");
    if (!in) return -1;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, in)) > 0) fwrite(buf, 1, n, out);
    fclose(in);
    fputc('\n', out);
    return 0;
}

static void write_theme(FILE *f)
{
    if (copy_file_into(f, THEME_RELATIVE_PATH) == 0) return;

    /* por si se corre desde otra carpeta*/
    char exe[1024];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (n > 0) {
        exe[n] = '\0';
        char *slash = strrchr(exe, '/');
        if (slash) {
            char path[1200];
            *slash = '\0';
            snprintf(path, sizeof path, "%s/%s", exe, THEME_RELATIVE_PATH);
            if (copy_file_into(f, path) == 0) return;
        }
    }
    fputs(FALLBACK_THEME, f);
}


/* Portada */

static void write_preamble(FILE *f, const ReportConfig *cfg)
{
    fputs("% _____________________________________________________________\n"
          "% Generado automáticamente por ./analizador -- no editar a mano\n"
          "% ______________________________________________________________\n"
          "\\documentclass[11pt,aspectratio=169]{beamer}\n"
          "\\usepackage[utf8]{inputenc}\n"
          "\\IfFileExists{lmodern.sty}{\\usepackage[T1]{fontenc}\\usepackage{lmodern}}{%\n"
          "  \\IfFileExists{courier.sty}{\\usepackage[T1]{fontenc}\\usepackage[scaled=0.92]{helvet}\\usepackage{courier}}{}}\n"
          "\\usepackage{booktabs}\n"
          "\\usepackage{tikz}\n"
          "\\usetikzlibrary{arrows.meta,positioning,calc,automata}\n"
          "\\usepackage{pgfplots}\n"
          "\\pgfplotsset{compat=1.16}\n", f);

    fputs("\\title[" REPORT_TITLE "]{" REPORT_TITLE "}\n", f);
    fputs("\\subtitle{" REPORT_SUBTITLE "}\n", f);
    fputs("\\author{", f);  put_escaped(f, cfg->group_members, 0, 0); fputs("}\n", f);
    fputs("\\date[", f);    put_escaped(f, cfg->course_term, 0, 40);  fputs("]{", f);
    put_escaped(f, cfg->course_term, 0, 0); fputs("}\n", f);
    fputs("\\institute{" REPORT_INSTITUTION "}\n\n", f);

    write_theme(f);
    fputs("\n\\begin{document}\n\n", f);
}

static void write_token_pill(FILE *f, const Token *t, Category c, int max_width)
{
    fprintf(f, "\\%s{", CATS[c].macro);
    put_lexeme(f, t, max_width);
    fputs("}", f);
}

static void write_title_frame(FILE *f, const ReportConfig *cfg)
{
    fputs("\\begin{frame}[plain]\n"
          "\\begin{tikzpicture}[remember picture,overlay]\n"
          "  \\fill[Navy] (current page.south west) rectangle (current page.north east);\n"
          "  \\fill[NavyLight] ($(current page.north east)+(-3.3cm,0)$) rectangle (current page.south east);\n"
          "  \\node[anchor=north west,align=left,text width=10.2cm,inner sep=0pt] at ($(current page.north west)+(1.1cm,-2.15cm)$)\n"
          "    {{\\color{white}\\fontsize{25}{30}\\selectfont\\bfseries " REPORT_TITLE "}\\\\[8pt]\n"
          "     {\\color{Accent}\\normalsize " REPORT_SUBTITLE "}};\n"
          "  \\node[anchor=south west,align=left,text width=10.2cm,inner sep=0pt] at ($(current page.south west)+(1.1cm,0.9cm)$)\n"
          "    {{\\color{white!55!Navy}\\tiny\\bfseries GRUPO DE TRABAJO}\\\\[1pt]\n"
          "     {\\color{white}\\small\\bfseries ", f);
    put_escaped(f, cfg->group_members, 0, 0);
    fputs("}\\\\[7pt]\n"
          "     {\\color{white!55!Navy}\\tiny\\bfseries SEMESTRE}\\\\[1pt]\n"
          "     {\\color{white}\\small ", f);
    put_escaped(f, cfg->course_term, 0, 0);
    fputs("}\\\\[7pt]\n"
          "     {\\color{white!55!Navy}\\scriptsize " REPORT_COURSE " \\textperiodcentered{} " REPORT_INSTITUTION "}};\n", f);


    fputs("\\end{tikzpicture}\n\\end{frame}\n\n", f);
}

typedef struct { const char *title; const char *desc; } SectionInfo;

static const SectionInfo SECTIONS[] = {
    { "El proceso de scanning" },
    { "La herramienta Flex",    "De expresiones regulares a un autómata en C" },
    { "Fuente preprocesada",    "El programa que entró al scanner, lexema por lexema" },
    { "Errores léxicos",        "Secuencias que no pertenecen a ninguna categoría" },
    { "Estadísticas",           "Histograma y gráfico de pastel" },
};
#define SECTION_COUNT ((int)(sizeof SECTIONS / sizeof SECTIONS[0]))

static void write_agenda_frame(FILE *f)
{
    fputs("\\begin{frame}{Contenido}\n\\vspace{0.4em}\n\\begin{tikzpicture}\n", f);
    for (int i = 0; i < SECTION_COUNT; i++) {
        double y = -1.05 * i;
        fprintf(f, "  \\node[circle,fill=%s,text=white,font=\\small\\bfseries,minimum size=0.75cm] (n%d) at (0,%.2f) {%d};\n",
                i % 2 ? "Teal" : "Navy", i, y, i + 1);
        fprintf(f, "  \\node[anchor=south west,font=\\bfseries\\small,text=Ink,inner sep=0pt] at ($(n%d.east)+(0.35cm,0.02cm)$) {%s};\n",
                i, SECTIONS[i].title);
        fprintf(f, "  \\node[anchor=north west,font=\\scriptsize,text=Muted,inner sep=0pt] at ($(n%d.east)+(0.35cm,-0.06cm)$) {%s};\n",
                i, SECTIONS[i].desc);
    }
    fputs("\\end{tikzpicture}\n\\end{frame}\n\n", f);
}

static void write_section_frame(FILE *f, int idx)
{
    fprintf(f, "\\section{%s}\n", SECTIONS[idx].title);
    fprintf(f,
        "\\begin{frame}[plain]\n"
        "\\begin{tikzpicture}[remember picture,overlay]\n"
        "  \\fill[Navy] (current page.south west) rectangle (current page.north east);\n"
        "  \\fill[Accent] ($(current page.west)+(0,-1.45cm)$) rectangle ++(3.6cm,0.09cm);\n"
        "  \\node[anchor=east,text=Accent,font=\\fontsize{58}{58}\\selectfont\\bfseries] (num) at ($(current page.west)+(3.6cm,0.05cm)$) {%02d};\n"
        "  \\node[anchor=south west,text=white,font=\\LARGE\\bfseries,inner sep=0pt] at ($(num.east)+(0.55cm,0.02cm)$) {%s};\n"
        "  \\node[anchor=north west,text=white!62!Navy,font=\\small,inner sep=0pt] at ($(num.east)+(0.55cm,-0.25cm)$) {%s};\n"
        "\\end{tikzpicture}\n"
        "\\end{frame}\n\n",
        idx + 1, SECTIONS[idx].title, SECTIONS[idx].desc);
}

/* Sección 1: scanning */

static void write_scanning_intro(FILE *f)
{
    fputs(
"\\begin{frame}{¿Qué es un analizador léxico?}\n"

"\\begin{columns}[T,onlytextwidth]\n"
"\\begin{column}{0.58\\textwidth}\n"
"\\begin{itemize}\\setlength{\\itemsep}{6pt}\n"
"\\item Lee el programa fuente como una simple \\alert{secuencia de caracteres}.\n"
"\\item Agrupa esos caracteres en \\textbf{lexemas}: las unidades mínimas con significado.\n"
"\\item Clasifica cada lexema en un \\textbf{token}: una categoría más sus atributos (texto, valor numérico, línea y columna).\n"
"\\item Descarta lo que no aporta significado: espacios, tabuladores y saltos de línea.\n"
"\\item Reporta los \\textbf{errores léxicos}: secuencias que no pertenecen a ninguna categoría del lenguaje.\n"
"\\end{itemize}\n"
"\\end{column}\n"
"\\begin{column}{0.38\\textwidth}\n"
"\\begin{block}{Idea clave}\n"
"\\small El analizador sintáctico nunca ve caracteres: pide los tokens de uno en uno llamando a \\texttt{get\\_token()}.\n"
"\\end{block}\n"
"\\vspace{6pt}\n"
"\\begin{exampleblock}{Un token es un registro}\n"
"\\small\\ttfamily \\textless{}categoría, lexema,\\\\ \\ línea, columna, valor\\textgreater{}\n"
"\\end{exampleblock}\n"
"\\end{column}\n"
"\\end{columns}\n"
"\\end{frame}\n\n", f);
}

static void write_text_to_tokens(FILE *f, const ReportConfig *cfg)
{
    size_t first = cfg->token_count;
    for (size_t i = 0; i < cfg->token_count; i++)
        if (classify(cfg->tokens[i].type) != CAT_NONE) { first = i; break; }

    fputs("\\begin{frame}{Del texto a los tokens}\n", f);
    if (first == cfg->token_count) {
        fputs("\\framesubtitle{La fuente analizada no contiene lexemas}\n"
              "\\centering\\vfill{\\large\\color{Muted} El archivo preprocesado está vacío: el scanner devolvió directamente \\texttt{EOF}.}\\vfill\n"
              "\\end{frame}\n\n", f);
        return;
    }

    long line = cfg->tokens[first].line;
    size_t idx[7];
    int n = 0, more = 0, budget = 60;
    for (size_t i = first; i < cfg->token_count && cfg->tokens[i].line == line; i++) {
        Category c = classify(cfg->tokens[i].type);
        if (c == CAT_NONE) continue;
        if (n == 6) { more = 1; break; }
        int w = cfg->tokens[i].lexeme ? unit_width_of(cfg->tokens[i].lexeme) : 1;
        if (w > 14) w = 15;
        if (n > 0 && budget - w < 0) { more = 1; break; }
        budget -= w;
        idx[n++] = i;
    }

    /* Texto original de la línea, reconstruido con las columnas reales. */
    Buf src = {0};
    long col = 1;
    int width = 0;
    for (size_t i = first; i < cfg->token_count && cfg->tokens[i].line == line; i++) {
        const Token *t = &cfg->tokens[i];
        if (classify(t->type) == CAT_NONE) continue;
        for (; col < t->column && width < 70; col++, width++) buf_puts(&src, "~");
        const char *lex = t->lexeme ? t->lexeme : "";
        int w = escape_into(&src, lex, 1, 70 - width > 0 ? 70 - width : 1);
        width += w;
        col = t->column + (long)strlen(lex);
        if (width >= 70) break;
    }

    
    fputs("\\centering\\vspace{0.8em}\n\\begin{tikzpicture}\n", f);
    fprintf(f, "\\node[draw=CodeRule,fill=CodeBg,rounded corners=3pt,inner sep=7pt,font=\\ttfamily\\small] (src) {%s};\n", buf_str(&src));
    buf_free(&src);
    fputs("\\node[lbl,anchor=south west,font=\\tiny\\bfseries] at ($(src.north west)+(0,3pt)$) {ENTRADA: SECUENCIA DE CARACTERES};\n", f);
    fputs("\\matrix[column sep=6pt,row sep=3pt,ampersand replacement=\\colsep,below=1.5cm of src] (m) {\n", f);
    for (int k = 0; k < n; k++) {
        const Token *t = &cfg->tokens[idx[k]];
        Category c = classify(t->type);
        fprintf(f, "%s\\node[pill=%s] {", k ? " \\colsep " : "  ", CATS[c].color);
        write_token_pill(f, t, c, 14);
        fputs("};", f);
    }
    if (more) fputs(" \\colsep \\node[font=\\small,text=Muted] {\\ldots};", f);
    fputs(" \\\\\n", f);
    for (int k = 0; k < n; k++) {
        Category c = classify(cfg->tokens[idx[k]].type);
        fprintf(f, "%s\\node[lbl,font=\\tiny\\bfseries,text=%s] {%s};", k ? " \\colsep " : "  ", CATS[c].color, CATS[c].singular);
    }
    if (more) fputs(" \\colsep \\node{};", f);
    fputs(" \\\\\n", f);
    for (int k = 0; k < n; k++) {
        const Token *t = &cfg->tokens[idx[k]];
        fprintf(f, "%s\\node[lbl,font=\\tiny\\ttfamily,align=center] {", k ? " \\colsep " : "  ");
        put_escaped(f, token_type_name(t->type), 0, 0);
        fprintf(f, "\\\\%ld:%ld};", t->line, t->column);
    }
    if (more) fputs(" \\colsep \\node{};", f);
    fputs(" \\\\\n};\n", f);
    fputs("\\draw[flow] (src.south) -- node[right,font=\\scriptsize,text=Muted] {\\texttt{get\\_token()}} (m.north);\n"
          "\\node[lbl,anchor=north,font=\\tiny\\bfseries] at ($(m.south)+(0,-4pt)$) {SALIDA: TOKENS CON CATEGORÍA, TIPO INTERNO Y POSICIÓN};\n"
          "\\end{tikzpicture}\n\\vfill\n"
          "{\\scriptsize\\color{Muted} Ejemplo tomado del archivo analizado: los espacios separan lexemas, pero no generan tokens.}\n"
          "\\end{frame}\n\n", f);
}

static void write_token_anatomy(FILE *f, const ReportConfig *cfg, const ReportData *d)
{
    fputs("\\begin{frame}{Tokens reales}\n"
          "\\framesubtitle{Primera aparición de cada categoría en la fuente analizada}\n"
          "\\centering\\small\n", f);
    int rows = 0;
    for (int c = 0; c < CAT_COUNT; c++) if (d->first_tok[c] >= 0) rows++;
    if (rows == 0) {
        fputs("\\vfill{\\color{Muted} No se reconocieron tokens en la fuente.}\\vfill\n\\end{frame}\n\n", f);
        return;
    }
    fputs("\\begin{tabular}{@{}l l l r r@{}}\n\\toprule\n"
          "\\textbf{Lexema} & \\textbf{Categoría} & \\textbf{Tipo interno} & \\textbf{Línea:col} & \\textbf{Valor numérico}\\\\\n\\midrule\n", f);
    for (int c = 0; c < CAT_COUNT; c++) {
        if (d->first_tok[c] < 0) continue;
        const Token *t = &cfg->tokens[d->first_tok[c]];
        fputs("{\\ttfamily ", f);
        write_token_pill(f, t, (Category)c, 16);
        fprintf(f, "} & %s & {\\ttfamily\\scriptsize ", CATS[c].singular);
        put_escaped(f, token_type_name(t->type), 0, 0);
        fprintf(f, "} & %ld:%ld & ", t->line, t->column);
        if (c == CAT_INT || c == CAT_FLOAT) fprintf(f, "%.6g", t->numeric_value);
        else fputs("{\\color{Muted}---}", f);
        fputs("\\\\\n", f);
    }
    fputs("\\bottomrule\n\\end{tabular}\n\\vfill\n"
          "{\\scriptsize\\color{Muted} Cada token es una estructura \\texttt{Token}: tipo, puntero a su propio lexema, línea, columna y valor numérico.}\n"
          "\\end{frame}\n\n", f);
}

static void write_disambiguation(FILE *f)
{
    fputs(
"\\begin{frame}{¿Cómo decide el scanner dónde termina un lexema?}\n"
"\\framesubtitle{Las dos reglas de desambiguación de Flex}\n"
"\\begin{columns}[T,onlytextwidth]\n"
"\\begin{column}{0.48\\textwidth}\n"
"\\begin{block}{1. Coincidencia más larga}\n"
"\\small Entre todas las reglas que coinciden, gana la que consume \\textbf{más caracteres}.\\par\\vspace{6pt}\n"
"\\centering\\begin{tikzpicture}[node distance=3pt]\n"
"\\node[pill=TokIdent] (a) {\\tokenIdent{x}};\n"
"\\node[pill=TokOp,right=of a] (b) {\\tokenOperator{\\textgreater{}=}};\n"
"\\node[pill=TokInt,right=of b] (c) {\\tokenInteger{1}};\n"
"\\node[lbl,right=6pt of c,text=Teal,font=\\scriptsize\\bfseries] {correcto};\n"
"\\node[pill=TokIdent,below=8pt of a] (d) {\\tokenIdent{x}};\n"
"\\node[pill=TokOp,right=of d] (e) {\\tokenOperator{\\textgreater{}}};\n"
"\\node[pill=TokOp,right=of e] (g) {\\tokenOperator{=}};\n"
"\\node[pill=TokInt,right=of g] (h) {\\tokenInteger{1}};\n"
"\\draw[TokErr,line width=1pt] ($(e.west)+(-2pt,0)$) -- ($(g.east)+(2pt,0)$);\n"
"\\node[lbl,right=6pt of h,text=TokErr,font=\\scriptsize\\bfseries] {incorrecto};\n"
"\\end{tikzpicture}\n"
"\\end{block}\n"
"\\end{column}\n"
"\\begin{column}{0.48\\textwidth}\n"
"\\begin{block}{2. Prioridad por orden}\n"
"\\small Si dos reglas coinciden con la \\textbf{misma longitud}, gana la que aparece primero en el archivo \\texttt{.l}.\\par\\vspace{6pt}\n"
"\\begin{itemize}\\scriptsize\\setlength{\\itemsep}{3pt}\n"
"\\item \\texttt{\\tokenKeyword{if}}: coincide con la palabra clave y con identificador; gana la palabra clave porque se declaró antes.\n"
"\\item \\texttt{\\tokenIdent{ifx}}: la regla de identificador consume más caracteres, así que la coincidencia más larga vence.\n"
"\\end{itemize}\n"
"\\end{block}\n"
"\\end{column}\n"
"\\end{columns}\n"
"\\vfill\n"
"{\\scriptsize\\color{Muted} Por eso los operadores largos (\\texttt{\\textgreater{}\\textgreater{}=}, \\texttt{==}, \\texttt{\\&\\&}) nunca se parten en operadores cortos.}\n"
"\\end{frame}\n\n", f);
}


/* Sección 2: Flex */

static void write_flex_frames(FILE *f)
{
    fputs(
"\\begin{frame}{¿Qué es Flex?}\n"
"\\framesubtitle{\\emph{Fast lexical analyzer generator}}\n"
"\\begin{columns}[T,onlytextwidth]\n"
"\\begin{column}{0.60\\textwidth}\n"
"\\begin{itemize}\\small\\setlength{\\itemsep}{3pt}\n"
"\\item Generador de analizadores léxicos, sucesor libre de \\texttt{lex}.\n"
"\\item Recibe un archivo \\texttt{.l} con \\textbf{expresiones regulares} y la \\textbf{acción en C} que se ejecuta al reconocer cada una.\n"
"\\item Produce \\texttt{lex.yy.c}: un scanner completo en C cuya función principal es \\texttt{yylex()}.\n"
"\\item Convierte las expresiones regulares en un \\alert{autómata finito determinista} codificado en tablas: el reconocimiento es lineal en el tamaño de la entrada.\n"
"\\item En cada acción el lexema está en \\texttt{yytext} y su longitud en \\texttt{yyleng}; la entrada se lee de \\texttt{yyin}.\n"
"\\end{itemize}\n"
"\\end{column}\n"
"\\begin{column}{0.36\\textwidth}\n"
"\\centering\n"
"\\begin{tikzpicture}[node distance=0.55cm]\n"
"\\node[stage=Teal,minimum width=3.2cm] (l) {\\texttt{scanner.l}\\\\[-1pt]{\\tiny\\mdseries reglas + acciones}};\n"
"\\node[stage=Navy,minimum width=3.2cm,below=of l] (c) {\\texttt{lex.yy.c}\\\\[-1pt]{\\tiny\\mdseries AFD en tablas + \\texttt{yylex()}}};\n"
"\\node[stage=NavyLight,minimum width=3.2cm,below=of c] (o) {\\texttt{lex.yy.o}\\\\[-1pt]{\\tiny\\mdseries objeto compilado}};\n"
"\\node[stage=Accent,text=Ink,minimum width=3.2cm,below=of o] (a) {\\texttt{./analizador}\\\\[-1pt]{\\tiny\\mdseries enlazado con \\texttt{-lfl}}};\n"
"\\draw[flow] (l) -- node[right,font=\\tiny,text=Muted] {\\texttt{flex}} (c);\n"
"\\draw[flow] (c) -- node[right,font=\\tiny,text=Muted] {\\texttt{gcc -c}} (o);\n"
"\\draw[flow] (o) -- node[right,font=\\tiny,text=Muted] {\\texttt{gcc}} (a);\n"
"\\end{tikzpicture}\n"
"\\end{column}\n"
"\\end{columns}\n"
"\\end{frame}\n\n", f);

    fputs(
"\\begin{frame}{Estructura de un archivo \\texttt{.l}}\n"
"\\framesubtitle{Tres secciones separadas por \\texttt{\\%\\%} (esquema general)}\n"
"\\centering\\vspace{0.3em}\n"
"\\begin{tikzpicture}[every node/.style={inner sep=5pt}]\n"
"\\node[stage=Navy,minimum width=2.9cm,minimum height=1.35cm] (s1) at (0,0) {Definiciones\\\\[-1pt]{\\tiny\\mdseries código C y macros regulares}};\n"
"\\node[anchor=west,draw=CodeRule,fill=CodeBg,rounded corners=3pt,text width=8.9cm,minimum height=1.35cm,align=left,font=\\ttfamily\\scriptsize] (b1) at ($(s1.east)+(0.3cm,0)$)\n"
"  {\\tokenOperator{\\%\\{} \\tokenKeyword{\\#include} \\tokenString{\"token.h\"} \\tokenOperator{\\%\\}}\\\\\n"
"   \\tokenIdent{DIGITO}~~~~\\tokenString{{[}0-9{]}}\\\\\n"
"   \\tokenIdent{ID}~~~~~~~~\\tokenString{{[}A-Za-z\\_{]}{[}A-Za-z0-9\\_{]}*}};\n"
"\\node[font=\\ttfamily\\small\\bfseries,text=Accent!80!black] (p1) at ($(b1.south)+(0,-0.28cm)$) {\\%\\%};\n"
"\\node[stage=Teal,minimum width=2.9cm,minimum height=1.75cm] (s2) at (0,-2.35) {Reglas\\\\[-1pt]{\\tiny\\mdseries patrón \\{ acción en C \\}}};\n"
"\\node[anchor=west,draw=CodeRule,fill=CodeBg,rounded corners=3pt,text width=8.9cm,minimum height=1.75cm,align=left,font=\\ttfamily\\scriptsize] (b2) at ($(s2.east)+(0.3cm,0)$)\n"
"  {\\tokenString{\\{DIGITO\\}+}~~~~~\\tokenSeparator{\\{} \\tokenKeyword{return} \\tokenIdent{entero}(); \\tokenSeparator{\\}}\\\\\n"
"   \\tokenString{\\{ID\\}}~~~~~~~~~~\\tokenSeparator{\\{} \\tokenKeyword{return} \\tokenIdent{clasificar}(\\tokenIdent{yytext}); \\tokenSeparator{\\}}\\\\\n"
"   \\tokenString{\"\\textgreater{}=\"}~~~~~~~~~~\\tokenSeparator{\\{} \\tokenKeyword{return} \\tokenIdent{operador}(); \\tokenSeparator{\\}}\\\\\n"
"   \\tokenString{.}~~~~~~~~~~~~~\\tokenSeparator{\\{} \\tokenKeyword{return} \\tokenIdent{error\\_lexico}(); \\tokenSeparator{\\}}};\n"
"\\node[font=\\ttfamily\\small\\bfseries,text=Accent!80!black] (p2) at ($(b2.south)+(0,-0.28cm)$) {\\%\\%};\n"
"\\node[stage=NavyLight,minimum width=2.9cm,minimum height=1.0cm] (s3) at (0,-4.45) {Código de usuario\\\\[-1pt]{\\tiny\\mdseries funciones auxiliares}};\n"
"\\node[anchor=west,draw=CodeRule,fill=CodeBg,rounded corners=3pt,text width=8.9cm,minimum height=1.0cm,align=left,font=\\ttfamily\\scriptsize] (b3) at ($(s3.east)+(0.3cm,0)$)\n"
"  {\\tokenIdent{Token} \\tokenIdent{get\\_token}(\\tokenKeyword{void}) \\tokenSeparator{\\{} \\tokenIdent{yylex}(); \\ldots{} \\tokenSeparator{\\}}};\n"
"\\end{tikzpicture}\n"
"\\end{frame}\n\n", f);

    fputs(
"\\begin{frame}{Arquitectura de este analizador}\n"
"\\framesubtitle{Cada módulo se comunica solo por su interfaz pública}\n"
"\\centering\\vspace{0.4em}\n"
"\\begin{tikzpicture}[every node/.style={minimum width=2.95cm,text width=2.75cm}]\n"
"\\node[stage=Muted] (in) at (0,0) {Archivo de entrada\\\\{\\tiny\\mdseries cualquier extensión}};\n"
"\\node[stage=Teal] (pp) at (3.6,0) {\\texttt{preprocess\\_file()}\\\\[-1pt]{\\tiny\\mdseries comentarios, \\#include, \\#define}};\n"
"\\node[stage=NavyLight] (tmp) at (7.2,0) {Temporal \\texttt{.tmp}\\\\{\\tiny\\mdseries entrada real del scanner}};\n"
"\\node[stage=Navy] (sc) at (10.8,0) {\\texttt{get\\_token()}\\\\{\\tiny\\mdseries scanner generado con Flex}};\n"
"\\node[stage=NavyLight] (tk) at (10.8,-2.0) {\\texttt{Token[]} + \\texttt{TokenStats}\\\\{\\tiny\\mdseries acumulados por \\texttt{main}}};\n"
"\\node[stage=Teal] (rp) at (7.2,-2.0) {\\texttt{generate\\_beamer\\_}\\\\[-2pt]\\texttt{report()}\\\\{\\tiny\\mdseries LaTeX + \\texttt{pgfplots}}};\n"
"\\node[stage=Navy] (tex) at (3.6,-2.0) {\\texttt{pdflatex}\\\\{\\tiny\\mdseries dos pasadas, sin intervención}};\n"
"\\node[stage=Accent,text=Ink] (pdf) at (0,-2.0) {PDF\\\\{\\tiny\\mdseries en modo presentación}};\n"
"\\draw[flow] (in) -- (pp); \\draw[flow] (pp) -- (tmp); \\draw[flow] (tmp) -- (sc);\n"
"\\draw[flow] (sc) -- (tk); \\draw[flow] (tk) -- (rp); \\draw[flow] (rp) -- (tex); \\draw[flow] (tex) -- (pdf);\n"
"\\end{tikzpicture}\n"
"\\vfill\n"
"\\begin{itemize}\\small\\setlength{\\itemsep}{3pt}\n"
"\\item \\texttt{get\\_token()} es independiente del resto del programa: puede reutilizarse en proyectos futuros.\n"
"\\item El reporte no vuelve a escanear: colorea la fuente con los mismos tokens que produjo el scanner.\n"
"\\end{itemize}\n"
"\\end{frame}\n\n", f);
}


/* Sección 3: fuente coloreada  */

static void write_legend_frame(FILE *f, const ReportData *d)
{
    fputs("\\begin{frame}{Cómo leer las diapositivas de código}\n"
          "\\framesubtitle{Cada categoría combina color, peso, inclinación y fondo}\n"
          "\\centering\\small\n"
          "\\begin{tabular}{@{}c l l r@{}}\n\\toprule\n"
          "\\textbf{Muestra} & \\textbf{Categoría} & \\textbf{Estilo} & \\textbf{En esta fuente}\\\\\n\\midrule\n", f);
    static const char *samples[CAT_COUNT] = {
        "while", "contador", "42", "3.14", "'a'", "\"hola\"", "+=", ";", "@"
    };
    for (int c = 0; c < CAT_COUNT; c++) {
        fprintf(f, "{\\ttfamily\\%s{", CATS[c].macro);
        put_escaped(f, samples[c], 1, 0);
        fprintf(f, "}} & %s & {\\scriptsize %s} & %lu\\\\\n", CATS[c].singular, CATS[c].style, d->cat_count[c]);
    }
    fputs("\\bottomrule\n\\end{tabular}\n\\vfill\n"
          "{\\scriptsize\\color{Muted} La columna de la izquierda numera las líneas del archivo preprocesado; "
          "$\\hookrightarrow$ indica que una línea larga continúa y $\\cdots$ resume varias líneas vacías.}\n"
          "\\end{frame}\n\n", f);
}

static long first_line_in(const Layout *L, size_t a, size_t b)
{
    for (size_t i = a; i < b; i++) if (L->rows[i].kind == ROW_LINE) return L->rows[i].number;
    return -1;
}

static long last_line_in(const Layout *L, size_t a, size_t b)
{
    for (size_t i = b; i > a; i--) if (L->rows[i - 1].kind == ROW_LINE) return L->rows[i - 1].number;
    return -1;
}

static size_t count_code_frames(const Layout *L)
{
    size_t frames = 0, start = 0;
    while (start < L->count) {
        size_t end = start + CODE_ROWS_PER_FRAME;
        if (end >= L->count) end = L->count;
        else {
            size_t e = end;
            while (e > start + 1 && L->rows[e].kind == ROW_CONT) e--;
            if (e > start + 1) end = e;
        }
        frames++;
        start = end;
    }
    return frames;
}

static void write_code_frames(FILE *f, const Layout *L)
{
    char gutter_sample[32];
    long maxl = L->max_line > 9 ? L->max_line : 9;
    snprintf(gutter_sample, sizeof gutter_sample, "%ld", maxl);
    for (char *p = gutter_sample; *p; p++) *p = '0';

    if (L->count == 0) {
        fputs("\\begin{frame}[t]{Fuente después del preproceso}\n"
              "\\framesubtitle{El archivo preprocesado no contiene lexemas}\n"
              "\\vfill\\centering{\\large\\color{Muted} No hay código que mostrar: la entrada quedó vacía después del preproceso.}\n"
              "\\codelegend\n\\end{frame}\n\n", f);
        return;
    }

    size_t total = count_code_frames(L), part = 0, start = 0;
    while (start < L->count) {
        size_t end = start + CODE_ROWS_PER_FRAME;
        if (end >= L->count) end = L->count;
        else {
            size_t e = end;
            while (e > start + 1 && L->rows[e].kind == ROW_CONT) e--;
            if (e > start + 1) end = e;
        }
        part++;
        long a = first_line_in(L, start, end), b = last_line_in(L, start, end);
        fputs("\\begin{frame}[t]{Fuente después del preproceso}\n\\framesubtitle{", f);
        if (a > 0 && b > a)       fprintf(f, "Líneas %ld--%ld", a, b);
        else if (a > 0)           fprintf(f, "Línea %ld", a);
        else                      fputs("Continuación", f);
        fprintf(f, " \\textperiodcentered{} parte %zu de %zu}\n", part, total);
        fprintf(f, "\\begin{codeblock}{%s}\n", gutter_sample);
        for (size_t i = start; i < end; i++) {
            const Row *r = &L->rows[i];
            switch (r->kind) {
            case ROW_LINE: fprintf(f, "\\codeline{%ld}{%s}\n", r->number, buf_str(&r->text)); break;
            case ROW_CONT: fprintf(f, "\\codecont{%s}\n", buf_str(&r->text)); break;
            case ROW_SKIP: fprintf(f, "\\codeskip{%ld}\n", r->number); break;
            }
        }
        fputs("\\end{codeblock}\n\\codelegend\n\\end{frame}\n\n", f);
        start = end;
    }
}


/* Sección 4: errores léxicos  */


static const char *error_hint(const Token *t)
{
    const unsigned char *s = (const unsigned char *)(t->lexeme ? t->lexeme : "");
    if (s[0] == '"')  return "Cadena sin cerrar o mal formada";
    if (s[0] == '\'') return "Literal de carácter inválido o sin cerrar";
    if (s[0] == '\0' || s[0] < 0x20 || s[0] >= 0x7F) return "Byte no textual";
    return "Símbolo que no pertenece a ninguna categoría";
}

static void write_error_frames(FILE *f, const ReportConfig *cfg, const ReportData *d)
{
    if (d->error_count == 0) {
        fputs("\\begin{frame}{Errores léxicos}\n"
              "\\framesubtitle{Resultado del análisis}\n"
              "\\centering\\vfill\n"
              "\\begin{tikzpicture}\n"
              "\\fill[Teal] (0,0) circle (0.85cm);\n"
              "\\draw[white,line width=3.2pt,line cap=round,line join=round] (-0.38,0.02) -- (-0.1,-0.27) -- (0.42,0.3);\n"
              "\\end{tikzpicture}\\par\\vspace{0.8em}\n"
              "{\\Large\\bfseries\\color{Ink} No se encontraron errores léxicos}\\par\\vspace{0.3em}\n"
              "{\\small\\color{Muted} Todos los lexemas de la fuente pertenecen a alguna categoría válida.}\n"
              "\\vfill\n\\end{frame}\n\n", f);
        return;
    }

    size_t listed = 0, total_listed = d->error_count < ERR_MAX_LISTED ? d->error_count : ERR_MAX_LISTED;
    size_t frames = (total_listed + ERR_ROWS_PER_FRAME - 1) / ERR_ROWS_PER_FRAME, part = 0;
    size_t i = 0;
    while (listed < total_listed) {
        part++;
        size_t in_frame = 0;
        fprintf(f, "\\begin{frame}{Errores léxicos detectados}\n"
                   "\\framesubtitle{%lu en total \\textperiodcentered{} parte %zu de %zu}\n"
                   "\\centering\\small\n"
                   "\\begin{tabular}{@{}r r l l@{}}\n\\toprule\n"
                   "\\textbf{Línea} & \\textbf{Col.} & \\textbf{Lexema} & \\textbf{Descripción}\\\\\n\\midrule\n",
                d->error_count, part, frames);
        for (; i < cfg->token_count && in_frame < ERR_ROWS_PER_FRAME && listed < total_listed; i++) {
            const Token *t = &cfg->tokens[i];
            if (classify(t->type) != CAT_ERROR) continue;
            fprintf(f, "%ld & %ld & {\\ttfamily\\tokenError{", t->line, t->column);
            put_lexeme(f, t, 28);
            fprintf(f, "}} & {\\scriptsize %s}\\\\\n", error_hint(t));
            in_frame++;
            listed++;
        }
        fputs("\\bottomrule\n\\end{tabular}\n\\vfill\n{\\scriptsize\\color{Muted} ", f);
        if (listed == total_listed && d->error_count > total_listed)
            fprintf(f, "\\ldots{} y %lu errores más. ", d->error_count - (unsigned long)total_listed);
        fputs("Todos los errores aparecen resaltados en las diapositivas de la fuente.}\n\\end{frame}\n\n", f);
    }
}


/* Sección 5: estadísticas*/


static void write_summary_table(FILE *f, const ReportData *d)
{
    unsigned long maxc = 1;
    for (int c = 0; c < CAT_COUNT; c++) if (d->cat_count[c] > maxc) maxc = d->cat_count[c];

    fputs("\\begin{frame}{Resumen por categoría}\n", f);
    fprintf(f, "\\framesubtitle{%lu tokens reconocidos \\textperiodcentered{} misma fuente de datos que las gráficas}\n", d->total);
    fputs("\\centering\\small\n\\begin{tabular}{@{}l r r l@{}}\n\\toprule\n"
          "\\textbf{Categoría} & \\textbf{Cantidad} & \\textbf{\\%} & \\textbf{Distribución}\\\\\n\\midrule\n", f);
    for (int c = 0; c < CAT_COUNT; c++) {
        unsigned long v = d->cat_count[c];
        double len = 4.2 * (double)v / (double)maxc;
        fprintf(f, "\\textcolor{%s}{\\rule{7pt}{7pt}}\\hspace{4pt}%s%s%s & %s%lu%s & %.1f & ",
                CATS[c].color, v ? "" : "{\\color{Muted}", CATS[c].plural, v ? "" : "}",
                v ? "" : "{\\color{Muted}", v, v ? "" : "}", pct(v, d->total));
        if (v) fprintf(f, "\\textcolor{%s}{\\rule{%.2fcm}{5pt}}", CATS[c].color, len < 0.05 ? 0.05 : len);
        fputs("\\\\\n", f);
    }
    fprintf(f, "\\midrule\n\\textbf{Total} & \\textbf{%lu} & %s & \\\\\n\\bottomrule\n\\end{tabular}\n",
            d->total, d->total ? "100.0" : "0.0");
    fputs("\\vfill\n{\\scriptsize\\color{Muted} No se cuentan espacios, saltos de línea ni el token de fin de archivo.}\n\\end{frame}\n\n", f);
}

static void write_no_data_chart(FILE *f, const char *title)
{
    fprintf(f, "\\begin{frame}{%s}\n\\centering\\vfill\n"
               "\\begin{tikzpicture}\\node[card,minimum width=8cm,minimum height=3cm] "
               "{{\\large\\bfseries\\color{Ink} Sin datos que graficar}\\\\[6pt]{\\small\\color{Muted} La fuente analizada no produjo tokens.}};\\end{tikzpicture}\n"
               "\\vfill\n\\end{frame}\n\n", title);
}

static unsigned long nice_step(unsigned long maxv)
{
    unsigned long target = maxv / 5 ? maxv / 5 : 1, mag = 1;
    while (mag * 10 <= target) mag *= 10;
    if (target <= mag) return mag;
    if (target <= 2 * mag) return 2 * mag;
    if (target <= 5 * mag) return 5 * mag;
    return 10 * mag;
}

static void write_histogram(FILE *f, const ReportData *d)
{
    int present[CAT_COUNT], n = 0;
    unsigned long maxv = 0;
    for (int c = 0; c < CAT_COUNT; c++)
        if (d->cat_count[c] > 0) { present[n++] = c; if (d->cat_count[c] > maxv) maxv = d->cat_count[c]; }
    if (n == 0) { write_no_data_chart(f, "Histograma de tokens por categoría"); return; }

    fputs("\\begin{frame}{Histograma de tokens por categoría}\n", f);
    fprintf(f, "\\framesubtitle{%lu tokens en %d categorías \\textperiodcentered{} datos reales}\n", d->total, n);
    fputs("\\centering\n\\begin{tikzpicture}\n\\begin{axis}[\n"
          "  width=0.97\\linewidth, height=0.74\\textheight, ybar, ymin=0,\n", f);
    fprintf(f, "  bar width=%.1fpt, xmin=0.4, xmax=%.1f,\n", n > 6 ? 20.0 : 26.0, n + 0.6);
    fputs("  xtick={", f);
    for (int k = 0; k < n; k++) fprintf(f, "%s%d", k ? "," : "", k + 1);
    fputs("}, xticklabels={", f);
    for (int k = 0; k < n; k++) fprintf(f, "%s{%s}", k ? "," : "", CATS[present[k]].plural);
    fputs("},\n", f);
    fprintf(f, "  ytick distance=%lu, enlarge y limits={upper,value=0.16},\n", nice_step(maxv));
    fputs("  x tick label style={font=\\scriptsize,rotate=22,anchor=north east,inner sep=1pt},\n"
          "  y tick label style={font=\\scriptsize}, xtick style={draw=none},\n"
          "  ylabel={Cantidad de tokens}, ylabel style={font=\\small,text=Muted},\n"
          "  axis lines*=left, axis line style={draw=CodeRule},\n"
          "  ymajorgrids, major grid style={dashed,draw=CodeRule},\n"
          "  nodes near coords, every node near coord/.append style={font=\\scriptsize\\bfseries,text=Ink},\n"
          "  scaled y ticks=false, yticklabel style={/pgf/number format/fixed,/pgf/number format/precision=0},\n"
          "]\n", f);
    for (int k = 0; k < n; k++)
        fprintf(f, "\\addplot[ybar,bar shift=0pt,fill=%s,draw=none] coordinates {(%d,%lu)};\n",
                CATS[present[k]].color, k + 1, d->cat_count[present[k]]);
    fputs("\\end{axis}\n\\end{tikzpicture}\n\\end{frame}\n\n", f);
}

typedef struct { int type; unsigned long count; } TypeCount;

static int cmp_typecount(const void *a, const void *b)
{
    const TypeCount *x = a, *y = b;
    if (x->count != y->count) return x->count < y->count ? 1 : -1;
    return x->type - y->type;
}

static void write_type_detail(FILE *f, const TokenStats *stats, const ReportData *d)
{
    TypeCount items[TOK_TYPE_COUNT];
    int n = 0;
    for (int t = 0; t < (int)TOK_TYPE_COUNT; t++)
        if (classify((TokenType)t) != CAT_NONE && stats->counts[t] > 0)
            items[n++] = (TypeCount){ t, stats->counts[t] };
    if (n == 0) return;
    qsort(items, (size_t)n, sizeof items[0], cmp_typecount);

    int shown = n > DETAIL_MAX_TYPES ? DETAIL_MAX_TYPES - 1 : n;
    unsigned long others = 0;
    for (int k = shown; k < n; k++) others += items[k].count;
    int bars = shown + (others ? 1 : 0);

    fputs("\\begin{frame}{Detalle por tipo interno de token}\n", f);
    fprintf(f, "\\framesubtitle{%d tipos distintos reportados por el scanner \\textperiodcentered{} %lu tokens}\n", n, d->total);
    fputs("\\centering\n\\begin{tikzpicture}\n\\begin{axis}[\n  xbar, xmin=0, y dir=reverse,\n", f);
    fprintf(f, "  width=0.86\\linewidth, height=%.2f\\textheight, bar width=%.1fpt,\n",
            bars < 5 ? 0.45 : 0.76, bars > 10 ? 6.5 : 9.0);
    fprintf(f, "  ymin=0.4, ymax=%.1f, ytick={", bars + 0.6);
    for (int k = 0; k < bars; k++) fprintf(f, "%s%d", k ? "," : "", k + 1);
    fputs("}, yticklabels={", f);
    for (int k = 0; k < shown; k++) {
        fputs(k ? ",{" : "{", f);
        put_escaped(f, token_type_name((TokenType)items[k].type), 0, 0);
        fputs("}", f);
    }
    if (others) fprintf(f, ",{Otros (%d tipos)}", n - shown);
    fputs("},\n  y tick label style={font=\\tiny\\ttfamily}, x tick label style={font=\\tiny}, ytick style={draw=none},\n"
          "  axis lines*=left, axis line style={draw=CodeRule}, xmajorgrids, major grid style={dashed,draw=CodeRule},\n"
          "  enlarge x limits={upper,value=0.12}, nodes near coords,\n"
          "  every node near coord/.append style={font=\\tiny\\bfseries,text=Ink},\n"
          "  scaled x ticks=false, xticklabel style={/pgf/number format/fixed,/pgf/number format/precision=0},\n", f);
    fprintf(f, "  xtick distance=%lu,\n]\n", nice_step(items[0].count));
    for (int k = 0; k < shown; k++)
        fprintf(f, "\\addplot[xbar,bar shift=0pt,fill=%s,draw=none] coordinates {(%lu,%d)};\n",
                CATS[classify((TokenType)items[k].type)].color, items[k].count, k + 1);
    if (others)
        fprintf(f, "\\addplot[xbar,bar shift=0pt,fill=CodeRule,draw=none] coordinates {(%lu,%d)};\n", others, bars);
    fputs("\\end{axis}\n\\end{tikzpicture}\n\\end{frame}\n\n", f);
}

static void write_pie(FILE *f, const ReportData *d)
{
    int n = 0;
    for (int c = 0; c < CAT_COUNT; c++) if (d->cat_count[c] > 0) n++;
    if (n == 0) { write_no_data_chart(f, "Gráfico de pastel de categorías léxicas"); return; }

    const double R_OUT = 1.0, R_IN = 0.55;
    fputs("\\begin{frame}{Gráfico de pastel de categorías léxicas}\n", f);
    fprintf(f, "\\framesubtitle{Mismas %d categorías y cantidades que el histograma \\textperiodcentered{} generado con \\texttt{pgfplots}}\n", n);
    fputs("\\centering\n\\begin{tikzpicture}\n\\begin{axis}[\n"
          "  hide axis, axis equal image, height=0.80\\textheight,\n"
          "  xmin=-1.08, xmax=1.08, ymin=-1.08, ymax=1.08,\n"
          "  legend style={at={(1.06,0.5)},anchor=west,draw=none,fill=none,font=\\scriptsize,row sep=2.5pt},\n"
          "  legend cell align=left,\n]\n", f);

    double start = 90.0;
    Buf labels = {0};
    for (int c = 0; c < CAT_COUNT; c++) {
        unsigned long v = d->cat_count[c];
        if (v == 0) continue;
        double frac = (double)v / (double)d->total;
        double end = start - frac * 360.0;
        int steps = (int)(frac * 180.0) + 2;
        fprintf(f, "\\addplot[fill=%s,draw=white,line width=1.1pt,area legend] coordinates {", CATS[c].color);
        for (int s = 0; s <= steps; s++) {
            double a = (start + (end - start) * s / steps) * REPORT_PI / 180.0;
            fprintf(f, "(%.4f,%.4f) ", R_OUT * r_cos(a), R_OUT * r_sin(a));
        }
        for (int s = steps; s >= 0; s--) {
            double a = (start + (end - start) * s / steps) * REPORT_PI / 180.0;
            fprintf(f, "(%.4f,%.4f) ", R_IN * r_cos(a), R_IN * r_sin(a));
        }
        fputs("} -- cycle;\n", f);
        fprintf(f, "\\addlegendentry{%s: %lu (%.1f\\,\\%%)}\n", CATS[c].plural, v, 100.0 * frac);
        if (frac >= 0.045) {
            double mid = (start + end) / 2.0 * REPORT_PI / 180.0, rl = (R_OUT + R_IN) / 2.0;
            buf_printf(&labels, "\\node[font=\\tiny\\bfseries,text=white] at (axis cs:%.4f,%.4f) {%.0f\\,\\%%};\n",
                       rl * r_cos(mid), rl * r_sin(mid), 100.0 * frac);
        }
        start = end;
    }
    fputs(buf_str(&labels), f);
    buf_free(&labels);
    fprintf(f, "\\node[align=center] at (axis cs:0,0) {{\\Large\\bfseries\\color{Ink}%lu}\\\\[-1pt]{\\tiny\\color{Muted}tokens}};\n", d->total);
    fputs("\\end{axis}\n\\end{tikzpicture}\n\\end{frame}\n\n", f);
}

static void write_run_summary(FILE *f, const ReportData *d, size_t code_frames)
{
    int top = -1;
    for (int c = 0; c < CAT_COUNT; c++)
        if (d->cat_count[c] > 0 && (top < 0 || d->cat_count[c] > d->cat_count[top])) top = c;

    fputs("\\begin{frame}{Resumen}\n\\framesubtitle{Archivo preprocesado: \\texttt{", f);
    put_escaped(f, d->source_name, 0, 60);
    fputs("}}\n\\centering\\vspace{1.2em}\n\\begin{tikzpicture}[node distance=0.32cm]\n", f);
    fprintf(f, "\\node[card] (c1) {{\\color{Navy}\\fontsize{22}{24}\\selectfont\\bfseries %lu}\\\\[5pt]{\\scriptsize\\color{Muted}tokens reconocidos}};\n", d->total);
    if (d->source_lines >= 0)
        fprintf(f, "\\node[card,right=of c1] (c2) {{\\color{Teal}\\fontsize{22}{24}\\selectfont\\bfseries %ld}\\\\[5pt]{\\scriptsize\\color{Muted}líneas preprocesadas}};\n", d->source_lines);
    else
        fputs("\\node[card,right=of c1] (c2) {{\\color{Teal}\\fontsize{22}{24}\\selectfont\\bfseries ---}\\\\[5pt]{\\scriptsize\\color{Muted}líneas preprocesadas}};\n", f);
    fprintf(f, "\\node[card,right=of c2] (c3) {{\\color{%s}\\fontsize{22}{24}\\selectfont\\bfseries %lu}\\\\[5pt]{\\scriptsize\\color{Muted}errores léxicos}};\n",
            d->error_count ? "TokErr" : "Teal", d->error_count);
    fprintf(f, "\\node[card,right=of c3] (c4) {{\\color{Accent!85!black}\\fontsize{22}{24}\\selectfont\\bfseries %zu}\\\\[5pt]{\\scriptsize\\color{Muted}diapositivas de código}};\n",
            code_frames);
    fputs("\\end{tikzpicture}\n\\vfill\n\\begin{itemize}\\small\\setlength{\\itemsep}{3pt}\n", f);
    if (top >= 0)
        fprintf(f, "\\item Categoría más frecuente: \\textbf{%s}, con %lu tokens (%.1f\\,\\%%).\n",
                CATS[top].plural, d->cat_count[top], pct(d->cat_count[top], d->total));
    else
        fputs("\\item La fuente no produjo tokens.\n", f);
    fprintf(f, "\\item Tasa de error léxico: \\textbf{%.1f\\,\\%%} de los tokens.\n", pct(d->error_count, d->total));
    fputs("\\item Histograma, pastel y tabla provienen de las mismas estadísticas acumuladas por \\texttt{main}.\n"
          "\\end{itemize}\n\\end{frame}\n\n", f);
}

static void write_closing_frame(FILE *f)
{
    fputs("\\begin{frame}[plain]\n"
          "\\begin{tikzpicture}[remember picture,overlay]\n"
          "  \\fill[Navy] (current page.south west) rectangle (current page.north east);\n"
          "  \\node[text=white,font=\\fontsize{34}{38}\\selectfont\\bfseries] at ($(current page.center)+(0,0.45cm)$) {¡Gracias!};\n"
          "\\end{tikzpicture}\n"
          "\\end{frame}\n\n", f);
}


/* Ejecución de procesos*/

static void redirect_std_to_devnull(void)
{
    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) close(fd);
    }
}


static int run_and_wait(char *const argv[], int timeout_s, int *status_out)
{
    *status_out = -1;
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        redirect_std_to_devnull();
        execvp(argv[0], argv);
        _exit(127);
    }
    struct timespec nap = { 0, 50 * 1000 * 1000 };
    long waited_ms = 0;
    for (;;) {
        int st;
        pid_t r = waitpid(pid, &st, WNOHANG);
        if (r == pid) {
            if (WIFEXITED(st)) { *status_out = WEXITSTATUS(st); return *status_out == 0 ? 0 : -1; }
            *status_out = -1;
            return -1;
        }
        if (r < 0 && errno != EINTR) return -1;
        if (waited_ms >= (long)timeout_s * 1000) {
            kill(pid, SIGKILL);
            waitpid(pid, &st, 0);
            *status_out = -2;
            return -1;
        }
        nanosleep(&nap, NULL);
        waited_ms += 50;
    }
}

/* Lanza un programa desacoplado (doble fork). Retorna 0 si exec tuvo
 * éxito; -1 si el programa no existe o no pudo iniciarse. */
static int spawn_detached(char *const argv[])
{
    int fds[2];
    if (pipe(fds) != 0) return -1;
    fcntl(fds[1], F_SETFD, FD_CLOEXEC);

    pid_t pid = fork();
    if (pid < 0) { close(fds[0]); close(fds[1]); return -1; }
    if (pid == 0) {
        close(fds[0]);
        pid_t g = fork();
        if (g != 0) _exit(g < 0 ? 1 : 0);
        setsid();
        redirect_std_to_devnull();
        execvp(argv[0], argv);
        int e = errno;
        if (write(fds[1], &e, sizeof e) < 0) { /* nada más que hacer */ }
        _exit(127);
    }
    close(fds[1]);
    waitpid(pid, NULL, 0);
    int e = 0;
    ssize_t r;
    do { r = read(fds[0], &e, sizeof e); } while (r < 0 && errno == EINTR);
    close(fds[0]);
    return r == 0 ? 0 : -1;
}

/* Copia la primera línea de error de LaTeX del .log (si la hay). */
static void first_latex_error(const char *log_path, char *out, size_t out_size)
{
    out[0] = '\0';
    FILE *f = fopen(log_path, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        char *p = strstr(line, ".tex:");
        int is_fle = 0;
        if (p) {
            char *q = p + 5;
            if (*q >= '0' && *q <= '9') { while (*q >= '0' && *q <= '9') q++; is_fle = (*q == ':'); }
        }
        if (line[0] == '!' || is_fle) {
            line[strcspn(line, "\r\n")] = '\0';
            snprintf(out, out_size, "%s", line);
            break;
        }
    }
    fclose(f);
}

/* Validación de rutas                                                 */

static int path_is_tex_safe(const char *p)
{
    for (const char *s = p; *s; s++)
        if (strchr(" \t\n%#$&{}\\~^\"'", *s)) return 0;
    return 1;
}

static int split_pdf_path(const char *pdf, char *dir, size_t dsz, char *job, size_t jsz)
{
    const char *slash = strrchr(pdf, '/');
    const char *base = slash ? slash + 1 : pdf;
    size_t blen = strlen(base);
    if (blen < 5 || strcmp(base + blen - 4, ".pdf") != 0) {
        set_error("La ruta del PDF debe terminar en .pdf: %s", pdf);
        return -1;
    }
    if (slash) {
        size_t dlen = (size_t)(slash - pdf);
        if (dlen == 0) dlen = 1;               /* "/archivo.pdf" */
        if (dlen >= dsz) { set_error("Ruta del PDF demasiado larga"); return -1; }
        memcpy(dir, pdf, dlen);
        dir[dlen] = '\0';
    } else {
        snprintf(dir, dsz, ".");
    }
    if (blen - 4 >= jsz) { set_error("Nombre del PDF demasiado largo"); return -1; }
    memcpy(job, base, blen - 4);
    job[blen - 4] = '\0';

    struct stat st;
    if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        set_error("La carpeta de salida del PDF no existe: %s", dir);
        return -1;
    }
    return 0;
}


/* Compilación y visor */


static int compile_pdf(const ReportConfig *cfg, const char *outdir, const char *job)
{
    char outarg[1100], jobarg[600], texarg[1100], logpath[1200];
    snprintf(outarg, sizeof outarg, "-output-directory=%s", outdir);
    snprintf(jobarg, sizeof jobarg, "-jobname=%s", job);
    snprintf(texarg, sizeof texarg, "%s%s", cfg->tex_path[0] == '/' ? "" : "./", cfg->tex_path);
    snprintf(logpath, sizeof logpath, "%s/%s.log", outdir, job);

    char *argv[] = { "pdflatex", "-interaction=nonstopmode", "-halt-on-error",
                     "-file-line-error", outarg, jobarg, texarg, NULL };

    /* Dos pasadas: la segunda resuelve el total de diapositivas y las
     * posiciones absolutas de TikZ (portada y separadores). */
    for (int pass = 1; pass <= 2; pass++) {
        int status;
        if (run_and_wait(argv, PDFLATEX_TIMEOUT_S, &status) != 0) {
            if (status == 127) {
                set_error("No se encontró 'pdflatex'. Instale TeX Live (texlive-latex-extra, texlive-pictures).");
            } else if (status == -2) {
                set_error("pdflatex excedió %d s en la pasada %d y fue detenido. Revise %s",
                          PDFLATEX_TIMEOUT_S, pass, logpath);
            } else {
                char detail[300];
                first_latex_error(logpath, detail, sizeof detail);
                set_error("pdflatex falló (código %d, pasada %d)%s%s. Log: %s",
                          status, pass, detail[0] ? ": " : "", detail, logpath);
            }
            return -1;
        }
    }

    struct stat st;
    if (stat(cfg->pdf_path, &st) != 0 || st.st_size == 0) {
        set_error("pdflatex terminó sin errores pero no se encontró el PDF en %s", cfg->pdf_path);
        return -1;
    }
    return 0;
}

static void open_viewer(const char *pdf_path)
{
    if (!getenv("DISPLAY") && !getenv("WAYLAND_DISPLAY")) {
        set_error("PDF generado; no se abrió el visor porque no hay sesión gráfica (DISPLAY).");
        return;
    }
    char path[1100];
    snprintf(path, sizeof path, "%s", pdf_path);

    char *evince[] = { "evince", "--presentation", path, NULL };
    char *okular[] = { "okular", "--presentation", path, NULL };
    char *xdg[]    = { "xdg-open", path, NULL };

    if (spawn_detached(evince) == 0) return;
    if (spawn_detached(okular) == 0) return;
    if (spawn_detached(xdg) == 0) {
        set_error("PDF generado; se abrió con xdg-open porque no se encontró evince (sin modo presentación).");
        return;
    }
    set_error("PDF generado; no se encontró ningún visor (evince, okular o xdg-open).");
}


/* Punto de entrada                                                    */


int generate_beamer_report(const ReportConfig *config, const TokenStats *stats)
{
    g_error[0] = '\0';

    if (!config || !stats) { set_error("Configuración o estadísticas nulas"); return -1; }
    if (!config->tex_path || !config->pdf_path || !config->processed_source_path) {
        set_error("Faltan rutas en la configuración del reporte");
        return -1;
    }
    if (config->token_count > 0 && !config->tokens) {
        set_error("token_count es %zu pero el arreglo de tokens es nulo", config->token_count);
        return -1;
    }

    ReportConfig cfg = *config;
    if (!cfg.group_members) cfg.group_members = "";
    if (!cfg.course_term)   cfg.course_term = "";

    char outdir[1024], job[512];
    if (split_pdf_path(cfg.pdf_path, outdir, sizeof outdir, job, sizeof job) != 0) return -1;
    if (!path_is_tex_safe(cfg.tex_path) || !path_is_tex_safe(outdir) || !path_is_tex_safe(job)) {
        set_error("Las rutas del .tex/.pdf no deben tener espacios ni caracteres especiales de LaTeX (%%, #, $, &, ~, ...)");
        return -1;
    }

    ReportData data;
    collect_data(&data, &cfg, stats);

    Layout layout;
    if (layout_build(&layout, cfg.tokens, cfg.token_count) != 0) {
        layout_free(&layout);
        set_error("Memoria insuficiente al maquetar la fuente coloreada");
        return -1;
    }

    FILE *f = fopen(cfg.tex_path, "w");
    if (!f) {
        set_error("No se pudo crear %s: %s", cfg.tex_path, strerror(errno));
        layout_free(&layout);
        return -1;
    }

    write_preamble(f, &cfg);
    write_title_frame(f, &cfg);
    write_agenda_frame(f);

    write_section_frame(f, 0);
    write_scanning_intro(f);
    write_text_to_tokens(f, &cfg);
    write_token_anatomy(f, &cfg, &data);
    write_disambiguation(f);

    write_section_frame(f, 1);
    write_flex_frames(f);

    write_section_frame(f, 2);
    write_legend_frame(f, &data);
    write_code_frames(f, &layout);

    write_section_frame(f, 3);
    write_error_frames(f, &cfg, &data);

    write_section_frame(f, 4);
    write_summary_table(f, &data);
    write_histogram(f, &data);
    write_type_detail(f, stats, &data);
    write_pie(f, &data);
    write_run_summary(f, &data, layout.count ? count_code_frames(&layout) : 1);
    write_closing_frame(f);

    fputs("\\end{document}\n", f);
    layout_free(&layout);

    int write_failed = ferror(f);
    if (fclose(f) != 0 || write_failed) {
        set_error("Error de escritura en %s", cfg.tex_path);
        return -1;
    }

    if (compile_pdf(&cfg, outdir, job) != 0) return -1;

    if (cfg.open_viewer) open_viewer(cfg.pdf_path);
    return 0;
}