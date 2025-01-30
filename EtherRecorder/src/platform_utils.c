#include "platform_utils.h"

#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <string.h>
#include <direct.h>
#define strcasecmp _stricmp
#else
#include <pthread.h>
#include <unistd.h>
#include <strings.h>
#endif

int platform_strcasecmp(const char* s1, const char* s2) {
    return strcasecmp(s1, s2);
}

uint64_t platform_strtoull(const char* str, char** endptr, int base) {
    return strtoull(str, endptr, base);
}

#ifdef _WIN32
char* platform_dirname(char* path) {
    static char buffer[PATH_MAX];
    char* last_slash = strrchr(path, '\\');
    if (last_slash) {
        size_t length = last_slash - path;
        strncpy(buffer, path, length);
        buffer[length] = '\0';
    }
    else {
        strcpy(buffer, ".");
    }
    return buffer;
}

int platform_mkdir(const char* path) {
    return _mkdir(path);
}
#else
char* platform_dirname(char* path) {
    return dirname(path);
}

int platform_mkdir(const char* path) {
    return mkdir(path, S_IRWXU);
}
#endif

void get_current_time(char *buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", t);
}

void lock_mutex(platform_mutex_t *mutex) {
    platform_mutex_lock(mutex);
}

void unlock_mutex(platform_mutex_t *mutex) {
    platform_mutex_unlock(mutex);
}

void stream_print(FILE *stream, const char *format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    int n = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (n > 0) {
        fwrite(buffer, 1, n, stream);
    }
}

void platform_sleep(unsigned int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000); // usleep takes microseconds
#endif
}

