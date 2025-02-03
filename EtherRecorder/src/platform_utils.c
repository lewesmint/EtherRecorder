#include "platform_utils.h"
#include "platform_mutex.h"

#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#define platform_mkdir(path) _mkdir(path)
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <limits.h>
#include <strings.h>
#define platform_mkdir(path) mkdir(path, 0755)
#endif

#ifdef _WIN32
const char PATH_SEPARATOR = '\\';
#else
const char PATH_SEPARATOR = '/'
#endif

uint64_t platform_strtoull(const char* str, char** endptr, int base) {
    return strtoull(str, endptr, base);
}

void get_current_time(char* buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    if (t != NULL) {
        strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", t);
    }
    else {
        snprintf(buffer, buffer_size, "1970-01-01 00:00:00");
    }
}

void init_mutex(PlatformMutex_T* mutex) {
    platform_mutex_init(mutex);
}

void lock_mutex(PlatformMutex_T* mutex) {
    platform_mutex_lock(mutex);
}

void unlock_mutex(PlatformMutex_T* mutex) {
    platform_mutex_unlock(mutex);
}

void stream_print(FILE* stream, const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    int n = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (n > 0) {
        fwrite(buffer, 1, (size_t)n, stream);
    }
}

void platform_sleep(unsigned int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

void sleep_seconds(double seconds) {
    unsigned int milliseconds = (unsigned int)(seconds * 1000);
    platform_sleep(milliseconds);
}

void sanitise_path(char* path) {
    if (!path) return;

    size_t len = strlen(path);
    while (len > 0 && isspace((unsigned char)path[len - 1])) {
        path[len - 1] = '\0';
        --len;
    }

    char* start = path;
    while (*start && isspace((unsigned char)*start)) {
        ++start;
    }
    if (start != path) {
        memmove(path, start, strlen(start) + 1);
        len = strlen(path);
    }

    while (len > 0 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
        path[len - 1] = '\0';
        --len;
    }

    for (char* p = path; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            *p = PATH_SEPARATOR;
        }
    }
}

void strip_directory_path(const char* full_file_path, char* directory_path, size_t size) {
    if (!full_file_path || !directory_path || size == 0) return;

    const char* last_sep = strrchr(full_file_path, PATH_SEPARATOR);
    if (last_sep != NULL) {
        size_t len = (size_t)(last_sep - full_file_path);
        if (len >= size) len = size - 1;
        strncpy(directory_path, full_file_path, len);
        directory_path[len] = '\0';
    }
    else {
        directory_path[0] = '\0';
    }
}

int create_directories(const char* path) {
    if (!path || !*path) return 0;

    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);

    size_t len = strlen(tmp);
    if (len > 0 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\')) {
        tmp[len - 1] = '\0';
    }

    for (char* p = tmp + 1; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            if (platform_mkdir(tmp) != 0 && errno != EEXIST) return -1;
            *p = PATH_SEPARATOR;
        }
    }
    return (platform_mkdir(tmp) != 0 && errno != EEXIST) ? -1 : 0;
}

void init_console(void) {
#ifdef _WIN32
    // Example: Disable Quick Edit mode on Windows
#endif
}

bool resolve_full_path(const char* filename, char* full_path, size_t size) {
#ifdef _WIN32
    return (_fullpath(full_path, filename, size) != NULL);
#else
    return (realpath(filename, full_path) != NULL);
#endif
}

/**
 * @brief Cross-platform implementation of strcasecmp()
 */
int platform_strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = tolower((unsigned char)*s1);
        char c2 = tolower((unsigned char)*s2);
        
        if (c1 != c2) {
            return c1 - c2;
        }
        
        s1++;
        s2++;
    }
    
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}