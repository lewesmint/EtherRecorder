#include "app_thread.h"

#include <string.h>
#include <stdbool.h>
#include <windows.h>
#include <stdio.h>

#include "platform_utils.h"
#include "platform_threads.h"
#include "app_config.h"
#include "logger.h"
#include "log_queue.h"
#include "platform_mutex.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#include <errno.h>
#endif

THREAD_LOCAL static const char *thread_label = NULL;

static CONDITION_VARIABLE logger_thread_condition;
static CRITICAL_SECTION logger_thread_mutex;
static bool logger_ready = false;
static bool shutdown_flag = false;

typedef enum WaitResult {
    APP_WAIT_SUCCESS = 0,
    APP_WAIT_TIMEOUT = 1,
    APP_WAIT_ERROR = -1
} WaitResult;

void* app_thread(AppThreadArgs_T* thread_args) {
    if (thread_args->init_func) {
        // Cast the result of init_func to bool and check if it returns true
        if ((WaitResult)thread_args->init_func(thread_args) != APP_WAIT_SUCCESS) {
            printf("[%s] Initialisation failed, exiting thread\n", thread_args->label);
            return NULL;
        }
    }
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
    logger_log(LOG_INFO, "Thread %s started msg:%d", thread_info->label, count++);
    for (int i = 0; i < LOG_ITERATIONS; i++) {
        for (int j = 0; j < LOG_FATAL; j++) { // Assuming LOG_FATAL is the highest log level
            LogLevel level = (LogLevel)(j % (LOG_FATAL + 1));
            logger_log(level, "%s logging %s msg:%d", get_thread_label(), log_level_to_string(level), count++);
            platform_sleep(50); // Simulate some work
            printf("[%s]We're still alive\n", get_thread_label());
        }
    }
    logger_log(LOG_INFO, "Thread %s completed msg:%d", thread_info->label, count++);
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

    // Simulate some delay to being ready 
    platform_sleep(8000);

    // Signal that the logger thread is ready
    EnterCriticalSection(&logger_thread_mutex);
    logger_ready = true;
    printf("Logger thread signaling condition variable\n");
    WakeAllConditionVariable(&logger_thread_condition);
    LeaveCriticalSection(&logger_thread_mutex);

    LogEntry_T entry;
    bool running = true; // Flag to handle graceful shutdown later

    while (running) {
        while (log_queue_pop(&log_queue, &entry) == 0) {
            log_immediately(entry.message);
        }

        platform_sleep(10);

        if (shutdown_flag) {
            running = false;
        }
    }

    logger_log(LOG_INFO, "Logger thread shutting down.");
    return NULL;
}

void request_shutdown() {
    shutdown_flag = true;
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

static void* init_wait_for_logger(void* arg);

AppThreadArgs_T all_threads[] = {
    // {
    //     .label = "LOGGER",
    //     .func = logger_thread_function,
    //     .data = NULL,
    //     .pre_create_func = pre_create_stub,
    //     .post_create_func = post_create_stub,
    //     .init_func = init_stub,
    //     .exit_func = exit_stub
    // },
    {
        .label = "CLIENT",
        .func = client_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_wait_for_logger,
        .exit_func = exit_stub
    },
    {
        .label = "SERVER",
        .func = server_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_wait_for_logger,
        .exit_func = exit_stub
    },
    {
        .label = "GENERIC-1",
        .func = generic_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_wait_for_logger,
        .exit_func = exit_stub
    },
    {
        .label = "GENERIC-2",
        .func = generic_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_wait_for_logger,
        .exit_func = exit_stub
    },
    {
        .label = "GENERIC-3",
        .func = generic_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_wait_for_logger,
        .exit_func = exit_stub
    },
    {
        .label = "LOGGER",
        .func = logger_thread_function,
        .data = NULL,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_stub,
        .exit_func = exit_stub
    }
};

int wait_for_condition_with_timeout(void* condition, void* mutex, int timeout_ms) {
#ifdef _WIN32
    if (!SleepConditionVariableCS((PCONDITION_VARIABLE)condition, (PCRITICAL_SECTION)mutex, timeout_ms)) {
        DWORD error = GetLastError();
        if (error == ERROR_TIMEOUT) {
            printf("Timeout occurred while waiting for condition variable\n");
            return APP_WAIT_TIMEOUT;
        } else {
            char* errorMsg = NULL;
            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&errorMsg, 0, NULL);
            printf("Error occurred while waiting for condition variable: %s\n", errorMsg);
            LocalFree(errorMsg);
            return APP_WAIT_ERROR;
        }
    }
    return APP_WAIT_SUCCESS;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }

    int rc = pthread_cond_timedwait((pthread_cond_t*)condition, (pthread_mutex_t*)mutex, &ts);
    if (rc == ETIMEDOUT) {
        printf("Timeout occurred while waiting for condition variable\n");
        return WAIT_TIMEOUT;
    }
    if (rc != 0) {
        printf("Error occurred while waiting for condition variable: %s\n", strerror(rc));
        return WAIT_ERROR;
    }
    return WAIT_SUCCESS;
#endif
}

static void* init_wait_for_logger(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    printf("[%s] Waiting for Logger\n", thread_info->label);
    // Wait for the logger thread to signal that it is ready
    EnterCriticalSection(&logger_thread_mutex);
    while (!logger_ready) {
        printf("[%s] Waiting on condition variable\n", thread_info->label);
        int result = wait_for_condition_with_timeout(&logger_thread_condition, &logger_thread_mutex, 5000); // 5 second timeout
        if (result == APP_WAIT_TIMEOUT) {
            printf("[%s] Timeout occurred while waiting for logger\n", thread_info->label);
            LeaveCriticalSection(&logger_thread_mutex);
            return (void*)APP_WAIT_TIMEOUT; // Indicate timeout
        } else if (result == APP_WAIT_ERROR) {
            printf("[%s] Error occurred while waiting for logger\n", thread_info->label);
            LeaveCriticalSection(&logger_thread_mutex);
            return (void*)APP_WAIT_ERROR; // Indicate error
        }
    }
    printf("[%s] Waiting for Logger over\n", thread_info->label);
    LeaveCriticalSection(&logger_thread_mutex);

    // Thread-specific initialization code
    // ...
    return (void*)APP_WAIT_SUCCESS;
}

void start_threads(void) {
    // Initialize the logger condition and mutex
    InitializeConditionVariable(&logger_thread_condition);
    InitializeCriticalSection(&logger_thread_mutex);

    int num_threads = sizeof(all_threads) / sizeof(all_threads[0]);

    for (int i = 0; i < num_threads; i++) {
        create_app_thread(&all_threads[i]);
    }
}
