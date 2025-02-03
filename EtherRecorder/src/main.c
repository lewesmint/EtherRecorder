#include <stdio.h>

#include "app_error.h"
#include "logger.h"
#include "app_config.h"
#include "platform_utils.h"
#include "platform_threads.h"
#include "log_queue.h"
#include "app_thread.h"

bool shutdown_flag = false;
CONDITION_VARIABLE shutdown_condition;
CRITICAL_SECTION shutdown_mutex;

static AppError init_app() {
    set_thread_label("MAIN");
    char config_load_result[LOG_MSG_BUFFER_SIZE];
    char logger_init_result[LOG_MSG_BUFFER_SIZE];

    init_console();
    // Load configuration, if config not found use defaults
    // This will only return false if the defaults cannot be set
    // for some reason.
    if (!load_config("config.ini", config_load_result)) {
        printf("Failed to initialize configuration: %s\n", config_load_result);
        return APP_CONFIG_ERROR;
    }

    // Initialise logger from configuration
    // This will only return false if we can't even log to stderr,
    // which is a serious problem, but not likely.
    // We need a logger before we try to launch threads, so
    // initially the logger works synchronously. We later 
    // launch a dedicated logger thread with a log queue.
    if (!init_logger_from_config(logger_init_result)) {
        printf("Configuration: %s\n", config_load_result);
        printf("Failed to initialise logger: %s\n", logger_init_result);
        return APP_LOGGER_ERROR;
    }
    logger_log(LOG_INFO, "Configuration: %s", config_load_result);
    logger_log(LOG_INFO, "Logger: %s", logger_init_result);
    return APP_EXIT_SUCCESS;
}

static AppError app_exit() {
    logger_log(LOG_INFO, "Exiting application");
    // Close the logger
    logger_close();

    // Deallocate memory used by the configuration
    // TODO: We might be able to deallocate earlier
    // if we're finished with the config, but it doesn't
    // use much memory so safe to do it here.
    free_config();
    return APP_EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // Initialize the shutdown condition and mutex
    InitializeConditionVariable(&shutdown_condition);
    InitializeCriticalSection(&shutdown_mutex);

    int app_error = init_app();
    if (app_error != APP_EXIT_SUCCESS) {
        return app_error;
    }

    // Now it's safe to log messages
    logger_log(LOG_ERROR, "Logger initialised successfully");
 
    // Start threads.
    // Successfully starting the logging thread will mean that logging will
    // utilise a log message queue, for asynchronous operation, to avoid threads 
    // blocking on logging.
    start_threads();

    // Wait for XX seconds
	// TODO get rid of this. It's just to test the logger queue
    platform_sleep(10000); // takes milliseconds

    // Request logger shutdown
    request_shutdown();

    // Wait for other threads to shut down gracefully
    int timeout_ms = 5000; // 5 seconds timeout
    if (!wait_for_shutdown(timeout_ms)) {
        // Forcefully terminate any remaining threads
        // This is platform-specific and should be handled carefully
        // For example, on Windows, you can use TerminateThread (not recommended)
    }

    // Signal the logger thread to shut down
    request_shutdown();

    // Shut down the logger thread
    logger_close();

    return app_exit();
}
