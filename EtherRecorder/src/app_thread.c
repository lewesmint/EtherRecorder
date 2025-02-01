#include "app_thread.h"

#include <string.h>

#include "platform_utils.h"
#include "platform_threads.h"
#include "app_config.h"
#include "logger.h"


THREAD_LOCAL static const char *thread_label = NULL;

PlatformCond_T logger_cond;
PlatformMutex_T logger_mutex;
int logger_ready = 0;

void* app_thread(AppThreadArgs_T* thread_args) {
    if (thread_args->init_func)
        thread_args->init_func(thread_args);
    thread_args->func(thread_args);
    if (thread_args->exit_func)
        thread_args->exit_func(thread_args);
    
    return NULL;
}

void create_app_thread(AppThreadArgs_T *thread) {
    if (thread->pre_create_func)
        thread->pre_create_func(thread);
    platform_thread_create(&thread->thread_id, (ThreadFunc_T)app_thread, thread);
    if (thread->post_create_func)
        thread->post_create_func(thread);
}

void set_thread_label(const char *label) {
    thread_label = label;
}

const char* get_thread_label() {
    return thread_label;
}

#define LOG_ITERATIONS 10000

void* generic_thread_function(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    set_thread_label(thread_info->label);
    long count = 0;
    logger_log(LOG_INFO, "Thread %s started msg:%d", get_thread_label(), count++);
    for (int i = 0; i < LOG_ITERATIONS; i++) {
        for (int j = 0; j < LOG_FATAL; j++) { // Assuming LOG_FATAL is the highest log level
            LogLevel level = (LogLevel)(j % (LOG_FATAL + 1));
            logger_log(level, "%s logging %s msg:%d", get_thread_label(), log_level_to_string(level), count++);
            platform_sleep(5); // Simulate some work
        }
    }
    logger_log(LOG_INFO, "Thread %s completed msg:%d", get_thread_label(), count++);
    return NULL;
}

void* client_thread_function(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    set_thread_label(thread_info->label);
    logger_log(LOG_INFO, "Client thread [%s] started", get_thread_label());
    // Client-specific functionality here
    return NULL;
}

void* server_thread_function(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    set_thread_label(thread_info->label);
    logger_log(LOG_INFO, "Server thread [%s] started", get_thread_label());
    // Server-specific functionality here
    return NULL;
}

void* logger_thread_function(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    set_thread_label(thread_info->label);
    logger_log(LOG_INFO, "Logger thread started");
    // Signal that the logger thread is ready
    platform_mutex_lock(&logger_mutex);
    logger_ready = 1;
    platform_cond_signal(&logger_cond);
    platform_mutex_unlock(&logger_mutex);
    // Logger-specific functionality here
    return NULL;
}

// Stub functions
void* pre_create_stub(void* arg ) {
    (void) arg;
    return 0;
}

void* post_create_stub(void* arg) {
    (void) arg;
    return 0;
}

void* init_stub(void* arg) {
    (void) arg;
    return 0; 
}

void* exit_stub(void* arg) {
    (void) arg;
    return 0;
}

AppThreadArgs_T all_threads[] = {
    {
        .label = "LOGGER",
        .func = logger_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_stub,
        .exit_func = exit_stub
    },
    {
        .label = "CLIENT",
        .func = client_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_stub,
        .exit_func = exit_stub
    },
    {
        .label = "SERVER",
        .func = server_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_stub,
        .exit_func = exit_stub
    },
    {
        .label = "GENERIC-1",
        .func = generic_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_stub,
        .exit_func = exit_stub
    },
    {
        .label = "GENERIC-2",
        .func = generic_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_stub,
        .exit_func = exit_stub
    },
    {
        .label = "GENERIC-3",
        .func = generic_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_stub,
        .exit_func = exit_stub
    }
};

void init_thread() {
    // Wait for the logger thread to signal that it is ready
    platform_mutex_lock(&logger_mutex);
    while (!logger_ready) {
        platform_cond_wait(&logger_cond, &logger_mutex);
    }
    platform_mutex_unlock(&logger_mutex);

    // Thread-specific initialization code
    // ...
}

void start_threads() {
    // Initialize the logger condition and mutex
    platform_cond_init(&logger_cond);
    platform_mutex_init(&logger_mutex);

    // Start the logger thread first
    create_app_thread(&all_threads[0]);

    int num_threads = sizeof(all_threads) / sizeof(all_threads[0]);

    for (int i = 1; i < num_threads; i++) {
        create_app_thread(&all_threads[i]);
    }
}
