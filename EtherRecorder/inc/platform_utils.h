#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "platform_mutex.h"

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
// #include <libgen.h>
#define PATH_SEPARATOR '\\'
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#define platform_mkdir(path) _mkdir(path)
// #define platform_dirname(path) _dirname(path)
#include <string.h>
#include <stdint.h>
#else
#include <sys/stat.h>
#include <libgen.h>
#define PATH_SEPARATOR '/'
#define platform_mkdir(path) mkdir(path, S_IRWXU)
#define platform_dirname(path) dirname(path)
#endif

//#ifdef _WIN32
//int create_directories(const char* path) {
//    char temp_path[2000];
//    snprintf(temp_path, sizeof(temp_path), "%s", path);
//
//    // Use PathRemoveFileSpec to get the directory part of the path
//    if (!PathRemoveFileSpecA(temp_path)) {
//        return -1;
//    }
//
//    if (platform_mkdir(temp_path) != 0 && errno != EEXIST) {
//        return -1;
//    }
//
//    return 0;
//}
//#else
///**
// * @brief Creates directories for the given path if they don't exist.
// * @param path The path for which to create directories.
// * @return 0 on success, -1 on failure.
// */
//int create_directories(const char *path) {
//    char temp_path[2000];
//    snprintf(temp_path, sizeof(temp_path), "%s", path);
//    char *dir = dirname(temp_path);
//
//    if (mkdir(dir, S_IRWXU) != 0 && errno != EEXIST) {
//        return -1;
//    }
//
//    return 0;
//}
//#endif // _WIN32

int platform_strcasecmp(const char* s1, const char* s2);

uint64_t platform_strtoull(const char* str, char** endptr, int base);

// Function to get the current time as a formatted string
void get_current_time(char *buffer, size_t buffer_size);

// Function to lock a mutex
void lock_mutex(platform_mutex_t *mutex);

// Function to unlock a mutex
void unlock_mutex(platform_mutex_t *mutex);

// Stream print function
void stream_print(FILE *stream, const char *format, ...);

// Function to sleep for a specified number of milliseconds
void platform_sleep(unsigned int milliseconds);


#endif // PLATFORM_UTILS_H