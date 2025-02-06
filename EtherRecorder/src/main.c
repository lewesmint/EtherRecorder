#include <stdio.h>
#include "platform_sockets.h"
#include "app_error.h"
#include "logger.h"
#include "app_config.h"
#include "platform_utils.h"
#include "platform_threads.h"
#include "log_queue.h"
#include "app_thread.h"

extern volatile bool shutdown_flag;

CONDITION_VARIABLE shutdown_condition;
CRITICAL_SECTION shutdown_mutex;

// Global critical section for protecting rand() calls.
// CRITICAL_SECTION rand_mutex;

// Call this once at the start of your program.


// // Call this once at program shutdown.
// void deleteRandCS(void) {
//     DeleteCriticalSection(&csRand);
// }



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char *progname) {
    printf("Usage: %s [-c <config_file>]\n", progname);
    printf("  -c <config_file>  Specify the configuration file (optional).\n");
    printf("  -h                Show this help message.\n");
}

//default config file
char config_file_name[MAX_PATH] = "config.ini";

bool parse_args(int argc, char *argv[]) {
    char *config_file = NULL;  // Optional config file

    // Iterate through arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && (i + 1) < argc) {
            config_file = argv[++i];  // Get the next argument as the filename
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return false;
        } else {
            printf("Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return false;
        }
    }
    return true;
}

static AppError init_app() {
    set_thread_label("MAIN");
    char config_load_result[LOG_MSG_BUFFER_SIZE];
    char logger_init_result[LOG_MSG_BUFFER_SIZE];

    init_console();
    // Load configuration, if config not found use defaults
    // This will only return false if the defaults cannot be set
    // for some reason.
    if (!load_config(config_file_name, config_load_result)) {
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

    /* Initialise sockets (WSAStartup on Windows, etc.) */
    initialise_sockets();
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
    if (!parse_args(argc, argv)) {
        return APP_EXIT_FAILURE;
    }

    // Initialize the shutdown condition and mutex
    InitializeConditionVariable(&shutdown_condition);
    InitializeCriticalSection(&shutdown_mutex);
    // InitializeCriticalSection(&rand_mutex);

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
    sleep_ms(10000); // takes milliseconds

    // Request logger shutdown
    request_shutdown();

    // Wait for other threads to shut down gracefully
    int timeout_ms = 50000; // 5 seconds timeout
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
