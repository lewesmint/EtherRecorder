#include "platform_mutex.h"

#ifdef _WIN32

/**
 * @copydoc platform_mutex_init
 */
int platform_mutex_init(platform_mutex_t *mutex) {
    *mutex = CreateMutex(NULL, FALSE, NULL);
    return (*mutex == NULL) ? -1 : 0;
}

/**
 * @copydoc platform_mutex_lock
 */
int platform_mutex_lock(platform_mutex_t *mutex) {
    return (WaitForSingleObject(*mutex, INFINITE) == WAIT_OBJECT_0) ? 0 : -1;
}

/**
 * @copydoc platform_mutex_unlock
 */
int platform_mutex_unlock(platform_mutex_t *mutex) {
    return (ReleaseMutex(*mutex) != 0) ? 0 : -1;
}

/**
 * @copydoc platform_mutex_destroy
 */
int platform_mutex_destroy(platform_mutex_t *mutex) {
    return (CloseHandle(*mutex) != 0) ? 0 : -1;
}

#else

/**
 * @copydoc platform_mutex_init
 */
int platform_mutex_init(platform_mutex_t *mutex) {
    return pthread_mutex_init(mutex, NULL);
}

/**
 * @copydoc platform_mutex_lock
 */
int platform_mutex_lock(platform_mutex_t *mutex) {
    return pthread_mutex_lock(mutex);
}

/**
 * @copydoc platform_mutex_unlock
 */
int platform_mutex_unlock(platform_mutex_t *mutex) {
    return pthread_mutex_unlock(mutex);
}

/**
 * @copydoc platform_mutex_destroy
 */
int platform_mutex_destroy(platform_mutex_t *mutex) {
    return pthread_mutex_destroy(mutex);
}

#endif