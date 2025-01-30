#include <stdio.h>
#include "logger.h"
#include "config.h"
#include "platform_utils.h"
#include "app_error.h" // Include the new header file

#define NUM_THREADS 5
#define LOG_ITERATIONS 10000

typedef struct {
    int id;
    char name[20];
} thread_info_t;

void* thread_function(void* arg) {
    thread_info_t* tinfo = (thread_info_t*)arg;
    long count = 0;
    logger_log(LOG_INFO, "Thread %s started msg:%d", tinfo->name, count++);
    for (int i = 0; i < LOG_ITERATIONS; i++) {
        for (int j = 0; j < LOG_FATAL; j++) { // Assuming LOG_FATAL is the highest log level
            LogLevel level = (LogLevel)(j % (LOG_FATAL + 1));
            logger_log(level, "Thread %s logging %s msg:%d", tinfo->name, log_level_to_string(level), count++);
            platform_sleep(5); // Simulate some work
        }
    }
    logger_log(LOG_INFO, "Thread %s completed msg:%d", tinfo->name, count++);
    return NULL;
}

static int init_app() {
    // Load configuration, if config not found use defaults
    // This will only return false if the defaults cannot be set
    // for some reason.
    if (!load_config("config.ini")) {
        return APP_CONFIG_ERROR;
    }

    // Initialize logger from configuration
    // This will only return false we can't even log to stderr
    if (!init_logger_from_config()) {
        return APP_LOGGER_ERROR;
    }

    // deallocate memory used by the configuration
    free_config();

    return APP_EXIT_SUCCESS;
}

static int app_exit() {
    logger_log(LOG_INFO, "Exiting application");
    // Close the logger
    logger_close();
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
    logger_log(LOG_ERROR, "Logger initialized successfully");

    // Create multiple threads
    pthread_t threads[NUM_THREADS];
    thread_info_t thread_infos[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_infos[i].id = i;
        snprintf(thread_infos[i].name, sizeof(thread_infos[i].name), "Thread-%d", i);
        pthread_create(&threads[i], NULL, thread_function, &thread_infos[i]);
    }
    
    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return app_exit();
    
}
