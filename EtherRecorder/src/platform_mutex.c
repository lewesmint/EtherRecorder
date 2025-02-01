#include "platform_mutex.h"

#ifdef _WIN32

/**
 * @brief 
 * 
 */ @copydoc platform_mutex_init
 */
int platform_mutex_init(PlatformMutex_T *mutex) {
    *mutex = CreateMutex(NULL, FALSE, NULL);
    return (*mutex == NULL) ? -1 : 0;
}

/**
 * @copydoc platform_mutex_lock
 */
int platform_mutex_lock(PlatformMutex_T *mutex) {
    return (WaitForSingleObject(*mutex, INFINITE) == WAIT_OBJECT_0) ? 0 : -1;
}

/**
 * @copydoc platform_mutex_unlock
 */
int platform_mutex_unlock(PlatformMutex_T *mutex) {
    return (ReleaseMutex(*mutex) != 0) ? 0 : -1;
}

/**
 * @copydoc platform_mutex_destroy
 */
int platform_mutex_destroy(PlatformMutex_T *mutex) {
    return (CloseHandle(*mutex) != 0) ? 0 : -1;
}

/**
 * @copydoc platform_cond_init
 */
int platform_cond_init(PlatformCondition_T *cond) {
    InitializeConditionVariable(cond);
    return 0;
}

/**
 * @copydoc platform_cond_wait
 */
int platform_cond_wait(PlatformCondition_T *cond, PlatformMutex_T *mutex) {
    return (SleepConditionVariableCS(cond, mutex, INFINITE) != 0) ? 0 : -1;
}

/**
 * @copydoc platform_cond_signal
 */
int platform_cond_signal(PlatformCondition_T *cond) {
    WakeConditionVariable(cond);
    return 0;
}

/**
 * @copydoc platform_cond_destroy
 */
int platform_cond_destroy(PlatformCondition_T *cond) {
    // No explicit destroy function for CONDITION_VARIABLE in Windows
    return 0;
}

#else

/**
 * @copydoc platform_mutex_init
 */
int platform_mutex_init(PlatformMutex_T *mutex) {
    return pthread_mutex_init(mutex, NULL);
}

/**
 * @copydoc platform_mutex_lock
 */
int platform_mutex_lock(PlatformMutex_T *mutex) {
    return pthread_mutex_lock(mutex);
}

/**
 * @copydoc platform_mutex_unlock
 */
int platform_mutex_unlock(PlatformMutex_T *mutex) {
    return pthread_mutex_unlock(mutex);
}

/**
 * @copydoc platform_mutex_destroy
 */
int platform_mutex_destroy(PlatformMutex_T *mutex) {
    return pthread_mutex_destroy(mutex);
}

/**
 * @copydoc platform_cond_init
 */
int platform_cond_init(PlatformCondition_T *cond) {
    return pthread_cond_init(cond, NULL);
}

/**
 * @copydoc platform_cond_wait
 */
int platform_cond_wait(PlatformCondition_T *cond, PlatformMutex_T *mutex) {
    return pthread_cond_wait(cond, mutex);
}

/**
 * @copydoc platform_cond_signal
 */
int platform_cond_signal(PlatformCondition_T *cond) {
    return pthread_cond_signal(cond);
}

/**
 * @copydoc platform_cond_destroy
 */
int platform_cond_destroy(PlatformCondition_T *cond) {
    return pthread_cond_destroy(cond);
}

#endif