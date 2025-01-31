#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

/**
 * @file logger.h
 * @brief Logger interface for logging messages to stderr and/or a file.
 */

/**
 * @enum LogOutput
 * @brief Defines the possible output destinations for logs.
 */
typedef enum LogOutput {
    LOG_OUTPUT_STDERR,  // Only screen/stderr
    LOG_OUTPUT_FILE,    // Only screen/stderr
    LOG_OUTPUT_BOTH     // Both screen/stderr and file
} LogOutput;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log levels.
 */
typedef enum {
    LOG_DEBUG, /**< Debug level */
    LOG_INFO,  /**< Info level */
    LOG_WARN,  /**< Warning level */
    LOG_ERROR, /**< Error level */
    LOG_FATAL  /**< Fatal level */
} LogLevel;

/**
 * @brief Initializes the logger.
 */
int init_logger_from_config();

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
 * @param level The log level of the message.
 * @param format The format string for the message.
 * @param ... The arguments for the format string.
 */
void logger_log(LogLevel level, const char *format, ...);


void logger_set_level(LogLevel level);

void logger_set_output(LogOutput output);

/**
 * @brief Closes the logger and releases any resources.
 */
void logger_close(void);

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H
