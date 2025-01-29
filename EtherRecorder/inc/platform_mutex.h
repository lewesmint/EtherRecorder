#ifndef PLATFORM_MUTEX_H
#define PLATFORM_MUTEX_H

#ifdef _WIN32
#include <windows.h>
typedef HANDLE platform_mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t platform_mutex_t;
#endif

/**
 * @brief Initializes a mutex.
 * 
 * @param mutex Pointer to the mutex to initialize.
 * @return int 0 on success, -1 on failure.
 */
int platform_mutex_init(platform_mutex_t *mutex);

/**
 * @brief Locks a mutex.
 * 
 * @param mutex Pointer to the mutex to lock.
 * @return int 0 on success, -1 on failure.
 */
int platform_mutex_lock(platform_mutex_t *mutex);

/**
 * @brief Unlocks a mutex.
 * 
 * @param mutex Pointer to the mutex to unlock.
 * @return int 0 on success, -1 on failure.
 */
int platform_mutex_unlock(platform_mutex_t *mutex);

/**
 * @brief Destroys a mutex.
 * 
 * @param mutex Pointer to the mutex to destroy.
 * @return int 0 on success, -1 on failure.
 */
int platform_mutex_destroy(platform_mutex_t *mutex);

#endif // PLATFORM_MUTEX_H