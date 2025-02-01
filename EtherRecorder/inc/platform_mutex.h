#ifndef PLATFORM_MUTEX_H
#define PLATFORM_MUTEX_H

#ifdef _WIN32
#include <windows.h>
typedef HANDLE PlatformMutex_T;
typedef CONDITION_VARIABLE PlatformCondition_T;
#else
#include <pthread.h>
typedef pthread_mutex_t PlatformMutex_T;
typedef pthread_cond_t PlatformCondition_T;
#endif

/**
 * @brief Initializes a mutex.
 * 
 * @param mutex Pointer to the mutex to initialize.
 * @return int 0 on success, -1 on failure.
 */
int platform_mutex_init(PlatformMutex_T *mutex);

/**
 * @brief Locks a mutex.
 * 
 * @param mutex Pointer to the mutex to lock.
 * @return int 0 on success, -1 on failure.
 */
int platform_mutex_lock(PlatformMutex_T *mutex);

/**
 * @brief Unlocks a mutex.
 * 
 * @param mutex Pointer to the mutex to unlock.
 * @return int 0 on success, -1 on failure.
 */
int platform_mutex_unlock(PlatformMutex_T *mutex);

/**
 * @brief Destroys a mutex.
 * 
 * @param mutex Pointer to the mutex to destroy.
 * @return int 0 on success, -1 on failure.
 */
int platform_mutex_destroy(PlatformMutex_T *mutex);

/**
 * @brief Initializes a condition variable.
 * 
 * @param cond Pointer to the condition variable to initialize.
 * @return int 0 on success, -1 on failure.
 */
int platform_cond_init(PlatformCondition_T *cond);

/**
 * @brief Waits on a condition variable.
 * 
 * @param cond Pointer to the condition variable to wait on.
 * @param mutex Pointer to the mutex to lock while waiting.
 * @return int 0 on success, -1 on failure.
 */
int platform_cond_wait(PlatformCondition_T *cond, PlatformMutex_T *mutex);

/**
 * @brief Signals a condition variable.
 * 
 * @param cond Pointer to the condition variable to signal.
 * @return int 0 on success, -1 on failure.
 */
int platform_cond_signal(PlatformCondition_T *cond);

/**
 * @brief Destroys a condition variable.
 * 
 * @param cond Pointer to the condition variable to destroy.
 * @return int 0 on success, -1 on failure.
 */
int platform_cond_destroy(PlatformCondition_T *cond);

#endif // PLATFORM_MUTEX_H