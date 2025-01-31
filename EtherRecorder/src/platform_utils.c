#include "platform_utils.h"

#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <string.h>
#include <direct.h>
#include <shlwapi.h>
// Link with Shlwapi.lib
#pragma comment(lib, "Shlwapi.lib")
#define strcasecmp _stricmp
#else // !_WIN32
#include <pthread.h>
#include <unistd.h>
#include <strings.h>
#endif // _WIN32 


uint64_t platform_strtoull(const char* str, char** endptr, int base) {
    return strtoull(str, endptr, base);
}

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

void sanitise_path(char *path)
{
    size_t len = strlen(path);
    char *end;

    // Trim trailing spaces
    while (len > 0 && isspace(path[len - 1])) {
        path[len - 1] = '\0';
        --len;
    }
    // Trim leading spaces
    end = path;
    while (*end && isspace(*end)) {
        ++end;
    }
    if (end != path) {
        memmove(path, end, strlen(end) + 1);
        len = strlen(path);
    }
    
    // Remove trailing slashes/backslashes
    while (len > 0 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
        path[len - 1] = '\0';
        --len;
    }

    // Ensure separators are correct for the platform
    for (char* p = path; *p; ++p) {
        if(*p == '/' || *p == '\\') {
            *p = PATH_SEPARATOR;
        }
    }
}

void platform_sleep(unsigned int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000); // usleep takes microseconds
#endif
}

// Function to strip the directory path from a full file path
void strip_directory_path(const char *full_file_path, char *directory_path, size_t size) {
    // Find the last occurance of PATH_SEPARATOR

    const char*  lastSep = strrchr(full_file_path, PATH_SEPARATOR);

    if (lastSep) {
        size_t len = lastSep - full_file_path; // Calculate the length of the directory path
        if (len >= size) {
            len = size -1;
        }
        strncpy(directory_path, full_file_path, len);
        directory_path[len] = '\0';
    } else {
        // No separator dound, so theres' no directory path
        directory_path[0] = '\0';
    }
}

// Platform agnostic function to recursively create directories for the given path if they don't exist 
int create_directories(const char *path) {
    if (!path || !*path) {
        return 0;
    }

    char tmp[MAX_PATH];
    char* p = NULL;
    size_t len;

    // Copy path for manipulation
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    // Remove trailing separator if persent
    if (tmp[len -1] == PATH_SEPARATOR) {
        tmp[len -1] = '\0';
    }

    // Iterate over the path and create directories as needed
    for (p = tmp + 1; *p; p++) {
        if (*p == PATH_SEPARATOR) {
            *p = '\0';
            if (platform_mkdir(tmp) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = PATH_SEPARATOR;
        }
    }
    if (platform_mkdir(tmp) != 0 && errno != EEXIST) {
        return -1;
    }

    return 0;
}

#ifdef _WIN32
static void disable_quick_edit_mode() {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD dwMode;
    GetConsoleMode(hInput, &dwMode);
    dwMode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hInput, dwMode);
}
#endif

void init_console() {
#ifdef _WIN32
    disable_quick_edit_mode(); 
#endif
}
