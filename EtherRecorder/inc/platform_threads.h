#ifndef PLATFORM_THREADS_H
#define PLATFORM_THREADS_H

#ifdef _WIN32
#include <windows.h>
typedef HANDLE platform_thread_t;
#else
#include <pthread.h>
typedef pthread_t platform_thread_t;
#endif

#ifdef _WIN32
#include <windows.h>
#define THREAD_LOCAL __declspec(thread)
typedef HANDLE platform_thread_t;
#else
#include <pthread.h>
#define THREAD_LOCAL __thread
typedef pthread_t platform_thread_t;
#endif


/**
 * @brief Template for thread functions.
 * @param arg The argument passed to the thread function.
 * @return The return value of the thread function.
 */
typedef void* (*thread_func_t)(void* arg);


/**
 * @brief Creates a new thread.
 *
 * @param thread Pointer to store the created thread handle.
 * @param func Function to be executed in the new thread.
 * @param arg Argument to pass to the thread function.
 * @return 0 on success, non-zero on failure.
 */
int platform_thread_create(platform_thread_t *thread, thread_func_t func, void *arg);

/**
 * @brief Waits for a thread to complete.
 *
 * @param thread The thread handle to wait for.
 * @param retval Pointer to store the return value of the thread.
 * @return 0 on success, non-zero on failure.
 */
int platform_thread_join(platform_thread_t thread, void **retval);

#endif // PLATFORM_THREADS_H
