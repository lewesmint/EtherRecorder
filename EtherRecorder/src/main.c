#include <stdio.h>

#include "app_error.h"
#include "logger.h"
#include "app_config.h"
#include "platform_utils.h"
#include "platform_threads.h"
#include "log_queue.h"
#include "app_thread.h"


static AppError init_app() {
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
    logger_log(LOG_INFO, "Configuration: %s\n", config_load_result);
    logger_log(LOG_INFO, "Logger: %s\n", logger_init_result);
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
    int app_error = init_app();
    if (app_error != APP_EXIT_SUCCESS) {
        return app_error;
    }

    // Now it's safe to log messages
    logger_log(LOG_ERROR, "Logger initialised successfully");
 
    // Start threads.
    // Successfully starting the logging thread will mean that logging will
    // utilise a log message queue, for asynchronus operation, to avoid threads 
    // blocking on logging.
    start_threads();

    return app_exit();
}
