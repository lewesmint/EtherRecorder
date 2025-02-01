#ifndef LOG_QUEUE_H
#define LOG_QUEUE_H

#include <stdatomic.h>
#include <stddef.h>

#include "logger.h"

#define LOG_QUEUE_SIZE 1024 // Size of the log queue
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
 * @brief Structure representing a log queue.
 */
typedef struct LogQueue_T {
    atomic_size_t head;
    atomic_size_t tail;
    LogEntry_T entries[LOG_QUEUE_SIZE];
} LogQueue_T;

extern LogQueue_T log_queue; // Declare the log queue

/**
 * @brief Initializes the log queue.
 * @param queue The log queue to initialize.
 */
void log_queue_init(LogQueue_T *queue);

/**
 * @brief Pushes a log entry onto the log queue.
 * @param queue The log queue.
 * @param level The log level of the message.
 * @param message The log message.
 * @return 0 on success, -1 if the queue is full.
 */
int log_queue_push(LogQueue_T *queue, LogLevel level, const char *message);

/**
 * @brief Pops a log entry from the log queue.
 * @param queue The log queue.
 * @param entry The log entry to populate.
 * @return 0 on success, -1 if the queue is empty.
 */
int log_queue_pop(LogQueue_T *queue, LogEntry_T *entry);

// /**
//  * @brief Log thread function implementation.
//  * @param arg The argument passed to the thread function.
//  * @return The return value of the thread function.
//  */
// void* log_thread_function_impl(void* arg);

#endif // LOG_QUEUE_H
