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
#define MAX_THREADS 100
#define APP_LOG_FILE_INDEX 0

#define CONFIG_LOG_PATH_KEY "log_file_path"
#define CONFIG_LOG_FILE_KEY "log_file_name"

typedef struct ThreadLogFile {
    char thread_label[MAX_PATH];
    FILE *log_fp;
    char log_file_name[MAX_PATH];
} ThreadLogFile;

static ThreadLogFile thread_log_files[MAX_THREADS + 1]; // +1 for the main application log file
static int thread_log_file_count = 0;

// static PlatformMutex_T thread_log_files_mutex; // Mutex for protecting thread_log_files

static LogLevel log_level = LOG_DEBUG; // Current log level
static LogOutput log_output = LOG_OUTPUT_BOTH; // Log output destination
static PlatformMutex_T logging_mutex; // Mutex for thread safety
static unsigned long long log_index = 0; // Log message index

// defaults if not read from the config file
static char log_file_path[MAX_PATH] = "";             // Log file path
static char log_file_name[MAX_PATH] = "log_file.log"; // Log file name
static off_t log_file_size = 10485760;                // Log file size before rotation

// Thread-specific log file
THREAD_LOCAL static char thread_log_file[MAX_PATH] = "";

static PlatformThread_T log_thread; // Logging thread
static bool logging_thread_started = false; // indicate whether the logger thread has started


/**
 * @brief Constructs the full log file name.
 * @param full_log_file_name The buffer to store the full log file name.
 * @param size The size of the buffer.
 * @param log_file_path The log file path.
 * @param log_file_name The log file name.
 */
static void construct_log_file_name(char *full_log_file_name, size_t size, const char *log_file_path, const char *log_file_name) {
    if (strlen(log_file_path) > 0) {
        snprintf(full_log_file_name, size, "%s%c%s", log_file_path, PATH_SEPARATOR, log_file_name);
    } else {
        strncpy(full_log_file_name, log_file_name, size - 1);
        full_log_file_name[size - 1] = '\0';
    }
    sanitise_path(full_log_file_name);
}


/**
 * @brief Sets the log file for the current thread.
 * @param filename The log file name to set.
 */
void set_log_thread_file(const char *label, const char *filename) {
    lock_mutex(&logging_mutex); // Lock the mutex

    if (thread_log_file_count >= MAX_THREADS) {
        // Maximum number of threads reached
        unlock_mutex(&logging_mutex); // Unlock the mutex
        return;
    }

    strncpy(thread_log_files[thread_log_file_count].thread_label, label, sizeof(thread_log_files[thread_log_file_count].thread_label) - 1);
    strncpy(thread_log_files[thread_log_file_count].log_file_name, filename, sizeof(thread_log_files[thread_log_file_count].log_file_name) - 1);
    // Don't open yet, leave it to the first log message in the context of the logger.
    thread_log_files[thread_log_file_count].log_fp = NULL;    
    thread_log_file_count++;

    unlock_mutex(&logging_mutex); // Unlock the mutex
}

/**
 * @brief Sets the log file for the current thread based on the config.
 * @param thread_label The name of the thread.
 */
void set_thread_log_file_from_config(const char *thread_label) {
    
    char file_config_key[MAX_PATH];
    snprintf(file_config_key, sizeof(file_config_key), "%s." CONFIG_LOG_FILE_KEY, thread_label);
    const char* config_thread_log_file = get_config_string("logger", file_config_key, NULL);
    const char* config_thread_log_path = get_config_string("logger", CONFIG_LOG_PATH_KEY, NULL);    

    if (config_thread_log_file) {
        if (config_thread_log_path) {
            char full_log_file_name[MAX_PATH];
            construct_log_file_name(full_log_file_name, sizeof(full_log_file_name), config_thread_log_path, config_thread_log_file);
            set_log_thread_file(thread_label, full_log_file_name);
        } else {
            set_log_thread_file(thread_label, config_thread_log_file);
        }
    }
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
 * 
 * @param log_file_full_name The full name of the log file.
 * @return false if the log file could not be opened, true if it was opened successfully.
 *         already open or opened successfully.
 */
// static bool open_log_file(const char* log_file_full_name) {
static bool open_log_file_if_needed(ThreadLogFile *thread_log_file) {
    if (thread_log_file->log_fp != NULL) {
        // its open already
        return true;
    }

    static int log_failure_count = 0; // Counter for log failures
    static int directory_creation_failure_count = 0; // Counter for directory creation failures

    char directory_path[MAX_PATH];

    // Strip the directory path from the full file path
    strip_directory_path(thread_log_file->log_file_name, directory_path, sizeof(directory_path));
    
    // Create the directories for the log file if they don't exist
    if (create_directories(directory_path) != 0) {
        if (directory_creation_failure_count < 5) {
            // don't go on forever
            stream_print(stderr, "Failed to create directory structure for logging: %s\n", directory_path);
            directory_creation_failure_count++;
        }
    }

    thread_log_file->log_fp = fopen(thread_log_file->log_file_name, "a");
    if (thread_log_file->log_fp == NULL) {
        if (log_failure_count == 0) {
            char error_message[LOG_MSG_BUFFER_SIZE];
            snprintf(error_message, sizeof(error_message), "Failed to open log file: %s\n", thread_log_file->log_file_name);
            fputs(error_message, stderr);
        }

        log_failure_count++;
        if (log_failure_count >= MAX_LOG_FAILURES) {
            char error_message[LOG_MSG_BUFFER_SIZE];
            snprintf(error_message, sizeof(error_message), "Unrecoverable failure to open log file: %s\n. Exiting\n", thread_log_file->log_file_name);
            fputs(error_message, stderr);
            exit(EXIT_FAILURE); // Exit if logging is impossible over 100 iterations
        }
        return false;
    } else {
        log_failure_count = 0; // Reset the counter on successful log file open
    }
    return true;
}

/**
 * @brief Rotates the log file if it exceeds the configured size.
 */
static void rotate_log_file_if_needed(ThreadLogFile *thread_log_file) {
    lock_mutex(&logging_mutex);
    struct stat st;
    if (stat(thread_log_file->log_file_name, &st) == 0 && st.st_size >= log_file_size) {
        fclose(thread_log_file->log_fp);
        char rotated_log_filename[512];
        generate_log_filename(rotated_log_filename, sizeof(rotated_log_filename));
        snprintf(rotated_log_filename + strlen(rotated_log_filename), sizeof(rotated_log_filename) - strlen(rotated_log_filename), ".old");
        rename(thread_log_file->log_file_name, rotated_log_filename);
        thread_log_file->log_fp = fopen(thread_log_file->log_file_name, "a");
        if (thread_log_file->log_fp == NULL) {
            unlock_mutex(&logging_mutex);
            return;
        }
    }
    unlock_mutex(&logging_mutex);
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

    // Strip off the newline character if it exists
    size_t len = strlen(message_buffer);
    if (len > 0 && message_buffer[len - 1] == '\n') {
        message_buffer[len - 1] = '\0';
    }

    if (get_thread_label()) {
        snprintf(buffer, buffer_size, "%010llu %s %s: [%s] %s", log_index++, time_str, log_level_to_string(level), get_thread_label(), message_buffer);
    } else {
        snprintf(buffer, buffer_size, "%010llu %s %s: %s", log_index++, time_str, log_level_to_string(level), message_buffer);
    }
}

/**
 * @brief Logs a message immediately to file and console.
 * @param level The log level of the message.
 * @param entry The formatted log message.
 */
static void log_immediately(LogEntry_T *entry) {
    lock_mutex(&logging_mutex);
    char *message = entry->message;
    char *thread_label = entry->thread_label;

    if (thread_label == NULL) {
        thread_label = get_thread_label();
    }   

    LogOutput prior_log_output = log_output;

    // Rotate the main log file if needed
    if (thread_log_files[APP_LOG_FILE_INDEX].log_fp != NULL) {
        rotate_log_file_if_needed(&thread_log_files[APP_LOG_FILE_INDEX]);
    }

    if (!open_log_file_if_needed(&thread_log_files[APP_LOG_FILE_INDEX])) {
        // if the log file cannot be opened, log to stderr only
        log_output = LOG_OUTPUT_STDERR;        
    }

    if (!message) {
        message = "Trying to log blank message";
    }

    ThreadLogFile tlf = thread_log_files[APP_LOG_FILE_INDEX];
    FILE *output_fp = thread_log_files[APP_LOG_FILE_INDEX].log_fp;
    const char *output_file_name = thread_log_files[APP_LOG_FILE_INDEX].log_file_name;
    
    // Check if the current thread has a specific log file
    if (thread_label != NULL) {
        for (int i = 1; i <= thread_log_file_count; i++) {
            if (platform_strcasecmp(thread_log_files[i].thread_label, thread_label) == 0) {
                // Rotate the thread log file if needed
                if (thread_log_files[i].log_fp != NULL) {
                    rotate_log_file_if_needed(&thread_log_files[i]);
                }

                if (!open_log_file_if_needed(&thread_log_files[i])) {
                    // TODO Handle error 
                }

                output_fp = thread_log_files[i].log_fp;
                output_file_name = thread_log_files[i].log_file_name;
				tlf = thread_log_files[i];
                break;
            }
        }
    }

    if (log_output == LOG_OUTPUT_FILE || log_output == LOG_OUTPUT_BOTH) {
        if (output_fp != NULL) {
            fputs(message, output_fp);
            fputs("\n", output_fp);
            fflush(output_fp);
        } else {
            // if the log file cannot be opened, log to stderr only
            printf("File Error: %s: msg:%s\n", output_file_name, message);
        }

    }
    if (log_output == LOG_OUTPUT_STDERR || log_output == LOG_OUTPUT_BOTH) {
        fputs(message, stderr);
        fputs("\n", stderr);
        fflush(stderr);
    }

    // TODO Look at logging to a TCP/UDP socket as an option

    log_output = prior_log_output;

    unlock_mutex(&logging_mutex);
}

/**
 * @brief Logs a message avoiding the queue
 */
void log_now(LogEntry_T *entry) {
    log_immediately(entry);
}


void logger_log(LogLevel level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    const char *this_thread_label = get_thread_label();
    const char *name = this_thread_label ? this_thread_label : "UNKNOWN";

    // Format the log message to include the thread name
    char log_message[256];
    snprintf(log_message, sizeof(log_message), "[%s] %s", name, format);

    char log_buffer[LOG_MSG_BUFFER_SIZE];
    format_log_message(log_buffer, sizeof(log_buffer), level, log_message, args);
    va_end(args);

    LogEntry_T entry;
    entry.level = level;
    // Copy the log label and message safely
    strncpy(entry.thread_label, name, THREAD_LABEL_SIZE - 1);
    strncpy(entry.message, log_buffer, LOG_MSG_BUFFER_SIZE - 1);
    entry.message[LOG_MSG_BUFFER_SIZE - 1] = '\0'; // Ensure null termination

    if (logging_thread_started) {
        // Push the log message to the queue; if full, log immediately
        if (!log_queue_push(&log_queue, &entry)) {
            log_immediately(&entry);
        }
    } else {
        log_immediately(&entry);
    }
}


/**
 * @brief Initializes the logger with the configured log file path, name, and size.
 */
bool init_logger_from_config(char *logger_init_result) {
    init_mutex(&logging_mutex);    
    lock_mutex(&logging_mutex);

    const char* config_log_file_path = get_config_string("logger", CONFIG_LOG_PATH_KEY, log_file_path);
    const char* config_log_file_name = get_config_string("logger", CONFIG_LOG_FILE_KEY, log_file_name);

    if (strlen(config_log_file_name) > 0) {
        if (strlen(config_log_file_path) > 0) {
            construct_log_file_name(thread_log_files[APP_LOG_FILE_INDEX].log_file_name, sizeof(thread_log_files[APP_LOG_FILE_INDEX].log_file_name), config_log_file_path, config_log_file_name);
        } else {
            strncpy(thread_log_files[APP_LOG_FILE_INDEX].log_file_name, config_log_file_name, sizeof(thread_log_files[APP_LOG_FILE_INDEX].log_file_name) - 1);
            thread_log_files[APP_LOG_FILE_INDEX].log_file_name[sizeof(thread_log_files[APP_LOG_FILE_INDEX].log_file_name) - 1] = '\0';
        }
        sanitise_path(thread_log_files[APP_LOG_FILE_INDEX].log_file_name);
    }
    // Increment thread_log_file_count to account for the main application log file
    thread_log_file_count++;
    log_file_size = get_config_int("logger", "log_file_size", log_file_size);

    // Initialize log queue
    log_queue_init(&log_queue);

    snprintf(logger_init_result, LOG_MSG_BUFFER_SIZE, "Logger initialised. App logging to %s", thread_log_files[APP_LOG_FILE_INDEX].log_file_name);
    logging_thread_started = true;
    unlock_mutex(&logging_mutex);
    return true;
}

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
    // Close all thread-specific log files
    for (int i = 0; i < thread_log_file_count; i++) {
        if (thread_log_files[i].log_fp) {
            fclose(thread_log_files[i].log_fp);
        }
    }
    thread_log_file_count = 0;

    unlock_mutex(&logging_mutex);

    if (logging_thread_started) {
        // Wait for logging thread to finish (it won't, so this is just for completeness)
        platform_thread_join(log_thread, NULL);
    }
}
