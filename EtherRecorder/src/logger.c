/**
 * @file logger.c
 * @brief Logging system for the project.
 *
 * Provides functionality for logging messages at various levels,
 * supporting output to a file and standard error.
 */

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
// #include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
// #include <libgen.h>

#include "platform_utils.h"
#include "config.h"

#define LOG_BUFFER_SIZE 1024 // Buffer size for log messages
#define MAX_LOG_FAILURES 100 // Maximum number of log failures before exiting

static FILE *log_fp = NULL; // Log file pointer
static LogLevel log_level = LOG_DEBUG; // Current log level
static LogOutput log_output = LOG_OUTPUT_BOTH; // Log output destination
static platform_mutex_t log_mutex; // Mutex for thread safety
static char log_file_full_name[PATH_MAX]; // Log file name' with path
static unsigned long long log_index = 0; // Log message index

// defaults if not read from the config file
static char log_file_path[PATH_MAX] = "";    // Log file path
static char log_file_name[256] = "foo.txt";  // Log file name
static off_t log_file_size = 10485760;       // Log file size before rotation


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
 * @copydoc log_level_to_string
 */
const char* log_level_to_string(LogLevel level)
{
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO ";
        case LOG_WARN: return "WARN ";
        case LOG_ERROR: return "ERROR";
        case LOG_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Creates directories for the given path if they don't exist.
 * @param path The path for which to create directories.
 * @return 0 on success, -1 on failure.
 */
static int create_directories(const char* path) {
    char temp_path[LOG_BUFFER_SIZE];
    snprintf(temp_path, sizeof(temp_path), "%s", path);
    char* dir = platform_dirname(temp_path);

    if (platform_mkdir(dir) != 0 && errno != EEXIST) {
        return -1;
    }

    return 0;
}

/**
 * @brief Opens the log file and manages the failure count.
 * @return 0 on success, -1 on failure.
 */
static int open_log_file() {
    static int log_failure_count = 0; // Counter for log failures

    if (create_directories(log_file_full_name) != 0) {
        fputs("Failed to create directories for log file\n", stderr);
        return -1;
    }

    log_fp = fopen(log_file_full_name, "a");
    if (!log_fp) {
        if (log_failure_count == 0) {
            char error_message[LOG_BUFFER_SIZE];
            snprintf(error_message, sizeof(error_message), "Failed to open log file: %s\n", log_file_full_name);
            fputs(error_message, stderr);
        }

        log_failure_count++;
        if (log_failure_count >= MAX_LOG_FAILURES) {
            char error_message[LOG_BUFFER_SIZE];
            snprintf(error_message, sizeof(error_message), "Unrecoverable failure to open log file: %s\n. Exiting\n", log_file_full_name);
            fputs(error_message, stderr);
            exit(EXIT_FAILURE); // Exit if logging is impossible over 100 iterations
        }
        return 1;
    } else {
        char message[LOG_BUFFER_SIZE];
        snprintf(message, sizeof(message), "Opened log file: %s", log_file_full_name);
        log_failure_count = 0; // Reset the counter on successful log file open
    }
    return 0;
}

/**
 * @brief Rotates the log file if it exceeds the configured size.
 * @copydoc rotate_log_file
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
            mutex_unlock(&log_mutex);
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

    char message_buffer[LOG_BUFFER_SIZE];
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);

    snprintf(buffer, buffer_size, "%010llu %s [%d] %s", log_index++, time_str, level, message_buffer);
}

/**
 * @brief Initializes the logger with the configured log file path, name, and size.
 * @copydoc init_logger_from_config
 */
int init_logger_from_config() {
    platform_mutex_init(&log_mutex);
    mutex_lock(&log_mutex);

    const char* config_log_file_path = get_config_string("logger", "log_file_path", log_file_path);
    if (log_file_path != config_log_file_path) {
        strncpy(log_file_path, config_log_file_path, sizeof(log_file_path) - 1);
        log_file_path[sizeof(log_file_path) - 1] = '\0';
    }

    const char* config_log_file_name = get_config_string("logger", "log_file_name", log_file_name);
    if (log_file_name != config_log_file_name) { 
        strncpy(log_file_name, config_log_file_name, sizeof(log_file_name) - 1);
        log_file_name[sizeof(log_file_name) - 1] = '\0';
    }

    if (strlen(log_file_path) > 0) {
        snprintf(log_file_full_name, sizeof(log_file_full_name), "%s%c%s", log_file_path, PATH_SEPARATOR, log_file_name);
    } else {
        snprintf(log_file_full_name, sizeof(log_file_full_name), "%s", log_file_name);
    }

    log_file_size = get_config_int("logger", "log_file_size", log_file_size);

    pthread_mutex_unlock(&log_mutex);
    return 1;
}

/**
 * @copydoc logger_set_level
 */
void logger_set_level(LogLevel level)
{
    log_level = level;
}

/**
 * @copydoc logger_set_output
 */
void logger_set_output(LogOutput output)
{
    log_output = output;
}

/**
 * @copydoc logger_log
 */
void logger_log(LogLevel level, const char *format, ...) {
    pthread_mutex_lock(&log_mutex);

    LogOutput prior_log_output = log_output;

    rotate_log_file(); // Check and rotate log file if necessary

    if (log_fp == NULL) {
        if (open_log_file() != 0) {
            // if the log file cannot be opened, log to stderr only
            log_output = LOG_OUTPUT_STDERR;
        }
    }

    char log_buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    format_log_message(log_buffer, sizeof(log_buffer), level, format, args);
    va_end(args);

    if (log_output == LOG_OUTPUT_FILE || log_output == LOG_OUTPUT_BOTH) {
        fputs(log_buffer, log_fp);
        fputs("\n", log_fp);
        fflush(log_fp);
    }
    if (log_output == LOG_OUTPUT_STDERR || log_output == LOG_OUTPUT_BOTH) {
        fputs(log_buffer, stderr);
        fputs("\n", stderr);
    }

    log_output = prior_log_output;

    pthread_mutex_unlock(&log_mutex);
}

/**
 * @copydoc logger_close
 */
void logger_close()
{
    pthread_mutex_lock(&log_mutex);
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
}
