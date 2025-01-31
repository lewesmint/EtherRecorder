#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include "platform_mutex.h"

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <windows.h>
#include <shlwapi.h>
// Link with Shlwapi.lib for _mkdir
#pragma comment(lib, "shlwapi.lib")
#else // !_WIN32
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>
#endif // _WIN32

#ifdef _WIN32
#define PATH_SEPARATOR '\\' // Windows path separator
#define platform_mkdir(path) _mkdir(path)
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif // PATH_MAX
#else // !_WIN32
#define PATH_SEPARATOR '/'
#define platform_mkdir(path) mkdir(path, 0755)
#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif // MAX_PATH
#endif // _WIN32

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

void sanitise_path(char *path);

void strip_directory_path(const char *full_file_path, char *directory_path, size_t size);

///**
// * @brief Creates directories for the given path if they don't exist.
// * @param path The path for which to create directories.
// * @return 0 on success, -1 on failure.
// */
int create_directories(const char *path);

void init_console();

#endif // PLATFORM_UTILS_H