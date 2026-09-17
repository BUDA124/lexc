#define _POSIX_C_SOURCE 200809L
#include "util.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#else
#include <unistd.h>
#endif

int ensure_directory_exists(const char *dir_path) {
    struct stat st = {0};
    if (stat(dir_path, &st) == -1) {
        if (mkdir(dir_path, 0755) != 0) {
            return -1;
        }
    }
    return 0;
}

void sanitize_filename(const char *src, char *dst, size_t max_len) {
    if (!src || !dst || max_len == 0) return;
    
    size_t i = 0, j = 0;
    while (src[i] != '\0' && j < max_len - 1) {
        if (isalnum((unsigned char)src[i]) || src[i] == '_') {
            dst[j++] = src[i];
        } else if (src[i] == '.') {
            dst[j++] = '_';
        }
        i++;
    }
    dst[j] = '\0';
}

int build_temp_path(const char *input_path, char *out_buf, size_t buf_size) {
    if (!input_path || !out_buf || buf_size == 0) return -1;

    const char *base = strrchr(input_path, '/');
    const char *base_win = strrchr(input_path, '\\');
    
    if (base_win && (!base || base_win > base)) {
        base = base_win;
    }
    
    if (base) {
        base++;
    } else {
        base = input_path;
    }
    
    char sanitized[256];
    sanitize_filename(base, sanitized, sizeof(sanitized));
    
    int pid = getpid();
    time_t now = time(NULL);
    
    int written = snprintf(out_buf, buf_size, "output/preprocessed_%s_%d_%lld.tmp", sanitized, pid, (long long)now);
    
    if (written < 0 || (size_t)written >= buf_size) {
        return -1;
    }
    return 0;
}
