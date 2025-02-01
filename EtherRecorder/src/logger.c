/**
 * @file logger.c
 * @brief Logging system for the project.
 *
 * Provides functionality for logging messages at various levels,
 * supporting output to a file and standard error.
 */

#include "logger.h"
#include "platform_threads.h"
#include "log_queue.h"
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

#include "platform_utils.h"
#include "app_thread.h"
#include "app_config.h"

#define MAX_LOG_FAILURES 100 // Maximum number of log failures before exiting

static FILE *log_fp = NULL; // Log file pointer
static LogLevel log_level = LOG_DEBUG; // Current log level
static LogOutput log_output = LOG_OUTPUT_BOTH; // Log output destination
static PlatformMutex_T logging_mutex; // Mutex for thread safety
static char log_file_full_name[PATH_MAX]; // Log file name' with path
static unsigned long long log_index = 0; // Log message index

// defaults if not read from the config file
static char log_file_path[MAX_PATH] = "";              // Log file path
static char log_file_name[MAX_PATH] = "log_file.log";  // Log file name
static off_t log_file_size = 10485760;                 // Log file size before rotation

// Thread-specific log file
THREAD_LOCAL static char thread_log_file[MAX_PATH] = "";

static PlatformThread_T log_thread; // Logging thread
static int logging_thread_started = 0; // Flag to indicate if logging thread has started
// static platform_thread_t main_thread_id; // Main thread ID

/**
 * @brief 
 * @brief Sets the log file for the current thread.
 * @param filename The log file name to set.
 */
void set_log_thread_file(const char *filename) {
    strncpy(thread_log_file, filename, sizeof(thread_log_file) - 1);
    thread_log_file[sizeof(thread_log_file) - 1] = '\0';
}

/**
 * @brief Generates a timestamped log filename.
 * @param buffer The buffer to store the filename.
 * @param size The size of the buffer.
 */
static void generate_log_filename(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "log_%Y-%m-%d.txt", t);
}

/**
 * @brief Converts log level to string.
 * @param level The log level.
 * @return The string representation of the log level.
 */
const char* log_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return  "INFO ";
        case LOG_WARN: return  "WARN ";
        case LOG_ERROR: return "ERROR";
        case LOG_FATAL: return "FATAL";
        default: return "UNKNN";
    }
}

/**
 * @brief Opens the log file and manages the failure count.
 * @return 0 on success, -1 on failure.
 */
static int open_log_file() {
    static int log_failure_count = 0; // Counter for log failures
    static int directory_creation_failure_count = 0; // Counter for directory creation failures

    char directory_path[MAX_PATH];

    // Strip the directory path from the full file path
    strip_directory_path(log_file_full_name, directory_path, sizeof(directory_path));
    
    // Create the directories for the log file if they don't exist
    if (create_directories(directory_path) != 0) {
        if (directory_creation_failure_count < 5) {
            // don't go on forever
            stream_print(stderr, "Failed to create directory structure for logging: %s\n", directory_path);
            directory_creation_failure_count++;
        }
    }

    log_fp = fopen(log_file_full_name, "a");
    if (!log_fp) {
        if (log_failure_count == 0) {
            char error_message[LOG_MSG_BUFFER_SIZE];
            snprintf(error_message, sizeof(error_message), "Failed to open log file: %s\n", log_file_full_name);
            fputs(error_message, stderr);
        }

        log_failure_count++;
        if (log_failure_count >= MAX_LOG_FAILURES) {
            char error_message[LOG_MSG_BUFFER_SIZE];
            snprintf(error_message, sizeof(error_message), "Unrecoverable failure to open log file: %s\n. Exiting\n", log_file_full_name);
            fputs(error_message, stderr);
            exit(EXIT_FAILURE); // Exit if logging is impossible over 100 iterations
        }
        return 1;
    } else {
        log_failure_count = 0; // Reset the counter on successful log file open
    }
    return 0;
}

/**
 * @brief Rotates the log file if it exceeds the configured size.
 */
static void rotate_log_file() {
    struct stat st;
    if (stat(log_file_full_name, &st) == 0 && st.st_size >= log_file_size) {
        fclose(log_fp);
        char rotated_log_filename[512];
        generate_log_filename(rotated_log_filename, sizeof(rotated_log_filename));
        snprintf(rotated_log_filename + strlen(rotated_log_filename), sizeof(rotated_log_filename) - strlen(rotated_log_filename), ".old");
        rename(log_file_full_name, rotated_log_filename);
        if (open_log_file() != 0) {
            unlock_mutex(&logging_mutex);
            exit(EXIT_FAILURE); // Exit if logging is critical
        }
    }
}

/**
 * @brief Formats the log message.
 * @param buffer The buffer to store the formatted message.
 * @param buffer_size The size of the buffer.
 * @param level The log level of the message.
 * @param format The format string (like printf).
 * @param args The arguments for the format string.
 */
static void format_log_message(char *buffer, size_t buffer_size, LogLevel level, const char *format, va_list args) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    char message_buffer[LOG_MSG_BUFFER_SIZE];
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);

    if (get_thread_label()) {
        snprintf(buffer, buffer_size, "%010llu %s %s: [%s] %s", log_index++, time_str, log_level_to_string(level), get_thread_label(), message_buffer);
    } else {
        snprintf(buffer, buffer_size, "%010llu %s %s: %s", log_index++, time_str, log_level_to_string(level), message_buffer);
    }
}

/**
 * @brief Logs a message immediately to file and console.
 * @param level The log level of the message.
 * @param message The formatted log message.
 */
void log_immediately(const char *message) {
    lock_mutex(&logging_mutex);

    LogOutput prior_log_output = log_output;

    rotate_log_file(); // Check and rotate log file if necessary

    if (log_fp == NULL) {
        if (open_log_file() != 0) {
            // if the log file cannot be opened, log to stderr only
            log_output = LOG_OUTPUT_STDERR;
        }
    }

    if (!message) {
        message = "Trying to log blank message";
    }

    if (log_output == LOG_OUTPUT_FILE || log_output == LOG_OUTPUT_BOTH) {
        fputs(message, log_fp);
        fputs("\n", log_fp);
        fflush(log_fp);
    }
    if (log_output == LOG_OUTPUT_STDERR || log_output == LOG_OUTPUT_BOTH) {
        fputs(message, stderr);
        fputs("\n", stderr);
    }

    log_output = prior_log_output;

    unlock_mutex(&logging_mutex);
}

/**
 * @brief Logs a message with the specified log level and format.
 * @param level The log level of the message.
 * @param format The format string (like printf).
 */
void logger_log(LogLevel level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const char *this_thread_label = get_thread_label();

    // Retrieve the thread name from the thread-local variable
    const char *name = this_thread_label ? this_thread_label : "unknown";

    // Format the log message to include the thread name
    char log_message[256];
    snprintf(log_message, sizeof(log_message), "[%s] %s", name, format);

    char log_buffer[LOG_MSG_BUFFER_SIZE];
    format_log_message(log_buffer, sizeof(log_buffer), level, log_message, args);
    va_end(args);

    // if (logging_thread_started && !platform_thread_equal(platform_thread_self(), main_thread_id)) {
    if (logging_thread_started) {
        // Push the log message to the queue
        log_queue_push(&log_queue, level, log_buffer);
    } else {
        // Log directly to file and console
        log_immediately(log_buffer);
    }
}

/**
 * @brief Initializes the logger with the configured log file path, name, and size.
 * @return 1 on success, 0 on failure.
 */
bool init_logger_from_config(char *logger_init_result) {
    init_mutex(&logging_mutex);
    lock_mutex(&logging_mutex);

    const char* config_log_file_path = get_config_string("logger", "log_file_path", log_file_path);
    if (log_file_path != config_log_file_path) {
        strncpy(log_file_path, config_log_file_path, sizeof(log_file_path) - 1);
        log_file_path[sizeof(log_file_path) - 1] = '\0';
    }

    sanitise_path(log_file_path);

    const char* config_log_file_name = get_config_string("logger", "log_file_name", log_file_name);
    if (log_file_name != config_log_file_name) { 
        strncpy(log_file_name, config_log_file_name, sizeof(log_file_name) - 1);
        log_file_name[sizeof(log_file_name) - 1] = '\0';
    }

    sanitise_path(log_file_name);

    if (strlen(log_file_path) > 0) {
        snprintf(log_file_full_name, sizeof(log_file_full_name), "%s%c%s", log_file_path, PATH_SEPARATOR, log_file_name);
    } else {
        snprintf(log_file_full_name, sizeof(log_file_full_name), "%s", log_file_name);
    }

    log_file_size = get_config_int("logger", "log_file_size", log_file_size);

    // Initialize log queue
    log_queue_init(&log_queue);

    snprintf(logger_init_result, LOG_MSG_BUFFER_SIZE, "Logger initialised. App logging to %s", log_file_full_name);
    unlock_mutex(&logging_mutex);
    return true;
}

/**
 * @brief Sets the log file for the current thread based on the config.
 * @param thread_name The name of the thread.
 */
void set_thread_log_file_from_config(const char *thread_name) {
    char config_key[MAX_PATH];
    snprintf(config_key, sizeof(config_key), "%s.log_file", thread_name);
    const char* config_thread_log_file = get_config_string("logger", config_key, NULL);
    if (config_thread_log_file) {
        set_log_thread_file(config_thread_log_file);
    }
}

/**
 * @brief Initializes the logger for a specific thread.
 * @param thread_name The name of the thread.
 */
void init_thread_logger(const char *thread_name) {
    // set_log_thread_label(thread_name);
    set_thread_log_file_from_config(thread_name);
}

// /**
//  * @brief Starts the logging thread.
//  */
// void start_logging_thread() {
//     lock_mutex(&log_mutex);
//     if (!logging_thread_started) {
//         platform_thread_create(&log_thread, log_thread_function, NULL);
//         logging_thread_started = 1;
//     }
//     unlock_mutex(&log_mutex);
// }

/**
 * @brief Sets the log level.
 * @param level The log level to set.
 */
void logger_set_level(LogLevel level) {
    log_level = level;
}

/**
 * @brief Sets the log output destination.
 * @param output The log output destination to set.
 */
void logger_set_output(LogOutput output) {
    log_output = output;
}

/**
 * @brief Closes the logger and releases resources.
 */
void logger_close() {
    lock_mutex(&logging_mutex);
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
    unlock_mutex(&logging_mutex);

    if (logging_thread_started) {
        // Wait for logging thread to finish (it won't, so this is just for completeness)
        platform_thread_join(log_thread, NULL);
    }
}
