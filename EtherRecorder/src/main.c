#include <stdio.h>

#include "app_error.h"
#include "logger.h"
#include "config.h"
#include "platform_utils.h"
#include "platform_threads.h"
#include "log_queue.h"
#include "app_thread.h"

#define NUM_THREADS 6
#define LOG_ITERATIONS 10000

void* generic_thread_function(void* arg) {
    app_thread_args_t* thread_info = (app_thread_args_t*)arg;

    set_thread_label(thread_info->label);
    long count = 0;
    logger_log(LOG_INFO, "Threa");
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
    app_thread_args_t* thread_info = (app_thread_args_t*)arg;
    set_thread_label(thread_info->label);
    logger_log(LOG_INFO, "Client thread [%s] started", get_thread_label());
    // Client-specific functionality here
    return NULL;
}

void* server_thread_function(void* arg) {
    app_thread_args_t* thread_info = (app_thread_args_t*)arg;
    set_thread_label(thread_info->label);
    logger_log(LOG_INFO, "Server thread [%s] started", get_thread_label());
    // Server-specific functionality here
    return NULL;
}

void* logger_thread_function(void* arg) {
    app_thread_args_t* thread_info = (app_thread_args_t*)arg;
    set_thread_label(thread_info->label);
    logger_log(LOG_INFO, "Logger thread started");
    // // Signal that tlgger is ready
    // sem_post(&logger_semaphore);
    // // Logger-specific functionality herehe lo
    return NULL;
}



static int init_app() {
    init_console();
    // Load configuration, if config not found use defaults
    // This will only return false if the defaults cannot be set
    // for some reason.
    if (!load_config("config.ini")) {
        return APP_CONFIG_ERROR;
    }

    // Initialise logger from configuration
    // This will only return false we can't even log to stderr
    // We needs a loggger before we try to launch threads, so
    // initially the logger works synchronously. We later 
    // launch a dedicated logger thread with a log queue.
    if (!init_logger_from_config()) {
        return APP_LOGGER_ERROR;
    }

    return APP_EXIT_SUCCESS;
}

static int app_exit() {
    logger_log(LOG_INFO, "Exiting application");
    // Close the logger
    logger_close();

    // Deallocate memory used by the configuration
    // TODO: We might be able to deallocate earlier
    // if we're finished with the config, but it doesn't
    // use much memory so safe to do it here.
    free_config();
    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    int app_error = init_app();
    if (app_error != APP_EXIT_SUCCESS) {
        return app_error;
    }

    // Now it's safe to log messages
    logger_log(LOG_ERROR, "Logger initialised successfully");

    app_thread_args_t all_threads[NUM_THREADS] = {
        {"loggger", logger_thread_function, 0, NULL, NULL, NULL, NULL, NULL},
        {"client1", client_thread_function, 0, NULL, NULL, NULL, NULL, NULL},
        {"server", server_thread_function, 0, NULL, NULL, NULL, NULL, NULL},
        {"generic-1", generic_thread_function, 0, NULL, NULL, NULL, NULL, NULL},
        {"generic-2", generic_thread_function,  0, NULL, NULL, NULL, NULL, NULL},
        {"generic-3", generic_thread_function, 0, NULL, NULL, NULL, NULL, NULL}
    };
 
    // Start threads.
    // Successfully starting the logging thread will mean that logging will
    // utilise a log message queue, for asynchronus operation, to avoid threads 
    // blocking on logging.
    start_threads(all_threads, NUM_THREADS);

    return app_exit();
}

