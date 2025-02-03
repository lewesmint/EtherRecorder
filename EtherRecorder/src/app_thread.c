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
static CRITICAL_SECTION logger_thread_mutex_in_app_thread;
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
    long count = 0;
    logger_log(LOG_INFO, "Thread %s started msg:%d", thread_info->label, count++);
    for (int i = 0; i < LOG_ITERATIONS; i++) {
        if (shutdown_flag) {
            logger_log(LOG_INFO, "Thread %s is shutting down early as instructed msg:%d", thread_info->label, count++);
            break;
        }
        for (int j = 0; j < LOG_FATAL; j++) { // Assuming LOG_FATAL is the highest log level
            LogLevel level = (LogLevel)(j % (LOG_FATAL + 1));
            logger_log(level, "%s logging %s msg:%d", get_thread_label(), log_level_to_string(level), count++);
            platform_sleep(50); // Simulate some work            
        }
    }
    logger_log(LOG_INFO, "Thread %s completed msg:%d", thread_info->label, count++);
    return NULL;
}

void* client_thread_function(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    
    // Client-specific functionality here

    logger_log(LOG_INFO, "Thread %s started", thread_info->label);
    for (int i = 0; i < LOG_ITERATIONS; i++) {
        if (shutdown_flag) {
            logger_log(LOG_INFO, "Thread %s is shutting down early as instructed", thread_info->label);
            break;
        }
        for (int j = 0; j < LOG_FATAL; j++) { // Assuming LOG_FATAL is the highest log level
            LogLevel level = (LogLevel)(j % (LOG_FATAL + 1));
            logger_log(level, "%s logging %s", get_thread_label(), log_level_to_string(level));
            platform_sleep(150); // Simulate some work            
        }
    }
    logger_log(LOG_INFO, "Thread %s completed", thread_info->label);
    return NULL;
}

void* server_thread_function(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    logger_log(LOG_INFO, "Server thread [%s] started", get_thread_label());

    // Server-specific functionality here

    for (int i = 0; i < LOG_ITERATIONS; i++) {
        if (shutdown_flag) {
            logger_log(LOG_INFO, "Thread %s is shutting down early as instructed", thread_info->label);
            break;
        }
        for (int j = 0; j < LOG_FATAL; j++) { // Assuming LOG_FATAL is the highest log level
            LogLevel level = (LogLevel)(j % (LOG_FATAL + 1));
            logger_log(level, "%s logging %s", get_thread_label(), log_level_to_string(level));
            platform_sleep(500); // Simulate some work            
        }
    }
    logger_log(LOG_INFO, "Thread %s completed", thread_info->label);
    return NULL;
}

void* logger_thread_function(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    set_thread_label(thread_info->label);
    logger_log(LOG_INFO, "Logger thread started");

    // Signal that the logger thread is ready
    EnterCriticalSection(&logger_thread_mutex_in_app_thread);
    logger_ready = true;
    printf("Logger thread signaling condition variable\n");
    // TODO make platfom indepdent for sanity's sake
    WakeAllConditionVariable(&logger_thread_condition);
    LeaveCriticalSection(&logger_thread_mutex_in_app_thread);
    

    LogEntry_T entry;
    bool running = true; // Flag to handle graceful shutdown later

    printf("Currently this is deliberately running the logger queue processing slowly to test queueing\n");
    while (running) {
        while (log_queue_pop_debug(&log_queue, &entry)) {
            log_now(entry.message);
            platform_sleep(40);
        }

        platform_sleep(1);

        if (shutdown_flag) {
            running = false;
        }
    }

    logger_log(LOG_INFO, "Logger thread shutting down.");
    return NULL;
}

void request_shutdown(void) {
    EnterCriticalSection(&shutdown_mutex);
    shutdown_flag = true;
    WakeAllConditionVariable(&shutdown_condition);
    LeaveCriticalSection(&shutdown_mutex);
}

bool wait_for_shutdown(int timeout_ms) {
    EnterCriticalSection(&shutdown_mutex);
    BOOL result = SleepConditionVariableCS(&shutdown_condition, &shutdown_mutex, timeout_ms);
    LeaveCriticalSection(&shutdown_mutex);
    return result;
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
            return APP_WAIT_TIMEOUT;
        } else {
            char* errorMsg = NULL;
            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&errorMsg, 0, NULL);
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
    set_thread_label(thread_info->label);    
    
    // Wait for the logger thread to signal that it is ready
    EnterCriticalSection(&logger_thread_mutex_in_app_thread);
    while (!logger_ready) {
        int result = wait_for_condition_with_timeout(&logger_thread_condition, &logger_thread_mutex_in_app_thread, 5000); // 5 second timeout
        if (result == APP_WAIT_TIMEOUT) {
            LeaveCriticalSection(&logger_thread_mutex_in_app_thread);
            return (void*)APP_WAIT_TIMEOUT; // Indicate timeout
        } else if (result == APP_WAIT_ERROR) {
            LeaveCriticalSection(&logger_thread_mutex_in_app_thread);
            return (void*)APP_WAIT_ERROR; // Indicate error
        }
    }
    LeaveCriticalSection(&logger_thread_mutex_in_app_thread);
    // Initialize the thread logger    
    set_thread_log_file_from_config(thread_info->label);
    logger_log(LOG_INFO, "Thread %s initialised", thread_info->label);

    return (void*)APP_WAIT_SUCCESS;
}

void start_threads(void) {
    // Initialize the logger condition and mutex
    InitializeConditionVariable(&logger_thread_condition);
    InitializeCriticalSection(&logger_thread_mutex_in_app_thread);

    int num_threads = sizeof(all_threads) / sizeof(all_threads[0]);

    for (int i = 0; i < num_threads; i++) {
        create_app_thread(&all_threads[i]);
    }
}
