#ifndef PLATFORM_THREADS_H
#define PLATFORM_THREADS_H

#ifdef _WIN32
#include <windows.h>
typedef HANDLE platform_thread_t;
#else
#include <pthread.h>
typedef pthread_t platform_thread_t;
#endif

typedef void* (*thread_func_t)(void*);

int platform_thread_create(platform_thread_t *thread, thread_func_t func, void *arg);
int platform_thread_join(platform_thread_t thread, void **retval);

#endif // PLATFORM_THREADS_H