#ifndef LOGGER_H
#define LOGGER_H
/**
 * @file logger.h
 * @brief Logger interface for logging messages to stderr and/or a file.
 */

#include <stdio.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @enum LogOutput
 * @brief Defines the possible output destinations for logs.
 */
typedef enum LogOutput {
    LOG_OUTPUT_STDERR,  // Only screen/stderr
    LOG_OUTPUT_FILE,    // Only file/stderr
    LOG_OUTPUT_BOTH     // Both screen/stderr and file
} LogOutput;

/**
 * @brief Log levels.
 */
typedef enum LogLevel{
    LOG_DEBUG, /**< Debug level */
    LOG_INFO,  /**< Info level */
    LOG_WARN,  /**< Warning level */
    LOG_ERROR, /**< Error level */
    LOG_FATAL  /**< Fatal level */
} LogLevel;


#define LOG_MSG_BUFFER_SIZE 1024 // Buffer size for log messages
#define THREAD_LABEL_SIZE 64 // Buffer size for thread labels

/**
 * @brief Structure representing a log entry.
 */
typedef struct LogEntry_T {
    LogLevel level;
    char message[LOG_MSG_BUFFER_SIZE];
    char thread_label[THREAD_LABEL_SIZE]; // Add thread label field
} LogEntry_T;

/**
 * @brief Initializes the logger.
 */
bool init_logger_from_config(char *logger_init_result);

// /**
//  * @brief Sets a thread-specific log label (e.g., "CLIENT" or "SERVER").
//  *
//  * @param thread_name The name of the thread.
//  */
// void set_log_thread_label(const char *thread_name);

// /**
//  * @brief Sets a thread-specific log file name.
//  *
//  * @param thread_name The name of the thread.
//  */
// void set_log_thread_file(const char *label, const char *filename);

/**
 * @brief Sets a thread-specific log file from configuration.
 *
 * @param thread_name The name of the thread.
 */
void set_thread_log_file_from_config(const char *thread_name);

/**
 * @brief Converts the log level to a string.
 *
 * @param level The log level to convert.
 * @return The string representation of the log level.
 */
const char* log_level_to_string(LogLevel level);

/**
 * @brief Logs a message with the specified log level.
 *
 * @param format The format string for the message.
 * @param ... The arguments for the format string.
 */
void logger_log(LogLevel level, const char *format, ...);

/**
 * @brief Logs a message now to file and console, avoiding the queue.
 */
void log_now(const LogEntry_T *entry);

/**
 * @brief Starts the logging thread.
 */
void start_logging_thread();

/**
 * @brief Closes the logger and releases any resources.
 */
void logger_close(void);

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H
