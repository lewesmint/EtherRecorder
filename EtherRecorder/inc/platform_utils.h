#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "platform_mutex.h"

#ifdef _WIN32
#include <windows.h>
#define PATH_SEPARATOR '\\'
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#include <string.h>
#include <stdint.h>
#else
#define PATH_SEPARATOR '/'
#endif

int platform_strcasecmp(const char* s1, const char* s2);

uint64_t platform_strtoull(const char* str, char** endptr, int base);

char* platform_dirname(char* path);

int platform_mkdir(const char* path);

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