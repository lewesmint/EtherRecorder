#include "app_thread.h"


#include <stdbool.h>
#include <windows.h>
#include <stdio.h>

#include "platform_utils.h"
#include "platform_threads.h"
#include "log_queue.h"
#include "logger.h"
#include "client_manager.h"
#include "server_manager.h"
#include "command_interface.h"
#include "app_config.h"


#define NUM_THREADS (sizeof(all_threads) / sizeof(all_threads[0]))

THREAD_LOCAL static const char *thread_label = NULL;

static CONDITION_VARIABLE logger_thread_condition;
static CRITICAL_SECTION logger_thread_mutex_in_app_thread;
volatile bool logger_ready = false;

extern volatile bool shutdown_flag;


// forward declaration required
void wait_for_all_other_threads_to_complete(void);


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
            sleep_ms(5000); // Simulate some work            
        }
    }
    logger_log(LOG_INFO, "Thread %s completed msg:%d", thread_info->label, count++);
    return NULL;
}

void* blank_thread_function(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;

    logger_log(LOG_INFO, "Thread %s started msg:", thread_info->label);
    while (!shutdown_flag) { // Assuming LOG_FATAL is the highest log level
        LogLevel level = (LogLevel)(LOG_FATAL);
        // 10000 seconds of sleeping between checks
        sleep_seconds(10000); // Simulate some work
    }

    logger_log(LOG_INFO, "Thread %s completed msg:%d", thread_info->label);
    return NULL;
}

// void* client_thread_function(void* arg) {
//     AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    
//     // Client-specific functionality here

//     logger_log(LOG_INFO, "Thread %s started", thread_info->label);
//     for (int i = 0; i < LOG_ITERATIONS; i++) {
//         if (shutdown_flag) {
//             logger_log(LOG_INFO, "Thread %s is shutting down early as instructed", thread_info->label);
//             break;
//         }
//         // pretending to do some work
//         for (int j = 0; j < LOG_FATAL; j++) { // Assuming LOG_FATAL is the highest log level
//             LogLevel level = (LogLevel)(j % (LOG_FATAL + 1));
//             logger_log(level, "%s logging %s", get_thread_label(), log_level_to_string(level));
//             platform_sleep(150); // Simulate some work            
//         }
//     }
//     logger_log(LOG_INFO, "Thread %s completed", thread_info->label);
//     return NULL;
// }

// void* server_thread_function(void* arg) {
//     AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
//     logger_log(LOG_INFO, "Server thread [%s] started", get_thread_label());

//     // Server-specific functionality here

//     for (int i = 0; i < LOG_ITERATIONS; i++) {
//         if (shutdown_flag) {
//             logger_log(LOG_INFO, "Thread %s is shutting down early as instructed", thread_info->label);
//             break;
//         }
//         for (int j = 0; j < LOG_FATAL; j++) { // Assuming LOG_FATAL is the highest log level
//             LogLevel level = (LogLevel)(j % (LOG_FATAL + 1));
//             logger_log(level, "%s logging %s", get_thread_label(), log_level_to_string(level));
//             platform_sleep(500); // Simulate some work            
//         }
//     }
//     logger_log(LOG_INFO, "Thread %s completed", thread_info->label);
//     return NULL;
// }


static char test_send_data [1000];

static ClientThreadArgs_T client_thread_args = {
    .server_hostname = "127.0.0.2",
    .send_test_data = false,
    .data = &test_send_data,
    .data_size = sizeof(test_send_data),
    .send_interval_ms = 2000,
    .port = 4200,
    .is_tcp = true
    };

// Stub functions
void* pre_create_stub(void* arg) {
    (void)arg;
    return 0;
}

void* post_create_stub(void* arg) {
    (void)arg;
    return 0;
}

void* init_stub(void* arg) {
    (void)arg;
    return 0;
}

void* exit_stub(void* arg) {
    (void)arg;
    return 0;
}

int wait_for_condition_with_timeout(void* condition, void* mutex, int timeout_ms) {
#ifdef _WIN32
    if (!SleepConditionVariableCS((PCONDITION_VARIABLE)condition, (PCRITICAL_SECTION)mutex, timeout_ms)) {
        DWORD error = GetLastError();
        if (error == ERROR_TIMEOUT) {
            return APP_WAIT_TIMEOUT;
        }
        else {
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

//TODO - simplify this by waiting for a signal,  set by the logger thread
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
        }
        else if (result == APP_WAIT_ERROR) {
            LeaveCriticalSection(&logger_thread_mutex_in_app_thread);
            return (void*)APP_WAIT_ERROR; // Indicate error
        }
    }
    LeaveCriticalSection(&logger_thread_mutex_in_app_thread);
    // Initialise the thread logger    
    set_thread_log_file_from_config(thread_info->label);
    logger_log(LOG_INFO, "Thread %s initialised", thread_info->label);

    return (void*)APP_WAIT_SUCCESS;
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

    while (running) {
        while (log_queue_pop(&global_log_queue, &entry)) {

            if (*entry.thread_label == '\0')
                printf("Logger thread processing log from: NULL\n");

            log_now(&entry);
            // sleep_ms(40);
        }

        sleep_ms(1);

        if (shutdown_flag) {
            running = false;
        }
    }

    // logger should only shut off when all other threads have shut down
    // so we need to hear from the other threads that they are done

    wait_for_all_other_threads_to_complete();
    
    logger_log(LOG_INFO, "Logger thread shutting down.");
    return NULL;
}


static AppThreadArgs_T all_threads[] = {
    {
        .label = "CLIENT",
        // .func = client_thread_function,
        .func = clientMainThread,
        .data = &client_thread_args,
        .pre_create_func = pre_create_stub,
        .post_create_func = post_create_stub,
        .init_func = init_wait_for_logger,
        .exit_func = exit_stub
    },
    // {
    //     .label = "SERVER",
    //     // .func = server_thread_function,
    //     .func = serverListenerThread,
    //     .data = &server_thread_args,
    //     .pre_create_func = pre_create_stub,
    //     .post_create_func = post_create_stub,
    //     .init_func = init_wait_for_logger,
    //     .exit_func = exit_stub
    // },
    // {
    //     .label = "GENERIC-1",
    //     // .func = generic_thread_function,
    //     .func = blank_thread_function,
    //     .data = NULL,
    //     .pre_create_func = pre_create_stub,
    //     .post_create_func = post_create_stub,
    //     .init_func = init_wait_for_logger,
    //     .exit_func = exit_stub
    // },
    // {
    //     .label = "GENERIC-2",
    //     // .func = generic_thread_function,
    //     .func = blank_thread_function,
    //     .data = NULL,
    //     .pre_create_func = pre_create_stub,
    //     .post_create_func = post_create_stub,
    //     .init_func = init_wait_for_logger,
    //     .exit_func = exit_stub
    // },
     {
         .label = "COMMAND_INTERFACE",
         .func = command_interface_thread,
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


void wait_for_all_other_threads_to_complete(void) {
    int num_threads = sizeof(all_threads) / sizeof(all_threads[0]);

    HANDLE threadHandles[NUM_THREADS];
    memset(threadHandles, 0, sizeof(threadHandles));

    DWORD j = 0;

    // Build an array of thread handles from structure.
    for (size_t i = 0; i < NUM_THREADS; i++) {
		if (all_threads[i].supressed ||
            str_cmp_nocase(all_threads[i].label, get_thread_label())) {
			continue;
		}
        threadHandles[j] = all_threads[i].thread_id;
    }

    // Wait for all threads to complete.
    DWORD waitResult = WaitForMultipleObjects(j, threadHandles, TRUE, INFINITE);

    if (waitResult == WAIT_FAILED) {
        DWORD err = GetLastError();
        logger_log(LOG_ERROR, "WaitForMultipleObjects failed: %lu\n", err);
        // Handle error appropriately.
    }
    else {
        // All threads have finished.
        logger_log(LOG_INFO, "Logger has seen all other threads complete");
    }

    // TODO drain the log queue

    LogEntry_T entry;

    while (log_queue_pop(&global_log_queue, &entry)) {

        if (*entry.thread_label == '\0')
            printf("Logger thread processing log from: NULL\n");

        log_now(&entry);
    }

    // we're done
}


static void* init_wait_for_logger(void* arg);

static ServerThreadArgs_T server_thread_args = {
    .port = 4200,
    .is_tcp = true
};

void wait_for_all_threads_to_complete(void) {
    uint32_t num_threads = sizeof(all_threads) / sizeof(all_threads[0]);

    HANDLE threadHandles[NUM_THREADS];
    memset(threadHandles, 0, sizeof(threadHandles));

    DWORD j = 0;
    // Build an array of thread handles from structure, careful of suppressed thread
    for (size_t i = 0; i < NUM_THREADS; i++) {
        if (all_threads[i].supressed) {
            continue;
        }
        threadHandles[j] = all_threads[i].thread_id;
        j++;
    }

	if (j == 0) {
		return;
	}
    // Wait for all threads to complete.
    DWORD waitResult = WaitForMultipleObjects(j, threadHandles, TRUE, INFINITE);

    if (waitResult == WAIT_FAILED) {
        DWORD err = GetLastError();
        fprintf(stderr, "WaitForMultipleObjects failed: %lu\n", err);
        // Handle error appropriately.
    } else {
        // All threads have finished.
        printf("All threads have completed.\n");
    }

    // Close the thread handles, done with them.
    for (size_t i = 0; i < num_threads; i++) {
        if (threadHandles[i] != NULL) {
            CloseHandle(threadHandles[i]);
        }
    }
}


void check_for_suppression(void) {
    const char* suppressed_list = get_config_string("debug", "suppress_threads", "");

    char suppressed_list_copy[CONFIG_MAX_VALUE_LENGTH];
    strncpy(suppressed_list_copy, suppressed_list, CONFIG_MAX_VALUE_LENGTH - 1);
    suppressed_list_copy[CONFIG_MAX_VALUE_LENGTH - 1] = '\0'; // Ensure null-termination

    char* context = NULL;
    char* token = strtok_s(suppressed_list_copy, ",", &context);

    while (token != NULL) {
        for (int i = 0; i < NUM_THREADS; i++) {
            if (str_cmp_nocase(all_threads[i].label, token) == 0) {
                all_threads[i].supressed = true;
            }
        }
        token = strtok_s(NULL, ",", &context); // Get next token
    }
}

void start_threads(void) {
    // Initialise the logger condition and mutex
    InitializeConditionVariable(&logger_thread_condition);
    InitializeCriticalSection(&logger_thread_mutex_in_app_thread);

    check_for_suppression();

    const char* get_config_string(const char* section, const char* key, const char* default_value);
    //int num_threads = sizeof(all_threads) / sizeof(all_threads[0]);

    for (int i = 0; i < NUM_THREADS; i++) {
        if (!all_threads[i].supressed) {
            create_app_thread(&all_threads[i]);
            // I need the handles, so I can wait for them to finish later
            HANDLE thread_handle = all_threads[i].thread_id;
        }
    }
}
