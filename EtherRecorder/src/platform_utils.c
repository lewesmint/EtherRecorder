#include "platform_utils.h"
#include <pthread.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
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

