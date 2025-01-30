#include "platform_threads.h"

#ifdef _WIN32
typedef struct {
    thread_func_t func;
    void *arg;
} thread_wrapper_t;

DWORD WINAPI thread_start(LPVOID arg) {
    thread_wrapper_t *wrapper = (thread_wrapper_t *)arg;
    wrapper->func(wrapper->arg);
    free(wrapper);
    return 0;
}

int platform_thread_create(platform_thread_t *thread, thread_func_t func, void *arg) {
    thread_wrapper_t *wrapper = (thread_wrapper_t *)malloc(sizeof(thread_wrapper_t));
    if (!wrapper) return -1;
    wrapper->func = func;
    wrapper->arg = arg;
    *thread = CreateThread(NULL, 0, thread_start, wrapper, 0, NULL);
    return (*thread == NULL) ? -1 : 0;
}

int platform_thread_join(platform_thread_t thread, void **retval) {
    (void)retval; // Unused
    return (WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0) ? 0 : -1;
}

#else

int platform_thread_create(platform_thread_t *thread, thread_func_t func, void *arg) {
    return pthread_create(thread, NULL, func, arg);
}

int platform_thread_join(platform_thread_t thread, void **retval) {
    return pthread_join(thread, retval);
}

#endif