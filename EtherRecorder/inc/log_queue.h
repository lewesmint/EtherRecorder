#ifndef LOG_QUEUE_H
#define LOG_QUEUE_H

typedef int placeholder_size_T;

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
    long head;
    long tail;
    LogEntry_T entries[LOG_QUEUE_SIZE];
} LogQueue_T;

extern LogQueue_T log_queue; // Declare the log queue

/**
 * @brief Initializes the log queue.
 * @param queue The log queue to initialize.
 */
void log_queue_init(LogQueue_T *queue);

// /**
//  * @brief Pushes a log entry onto the log queue.
//  * @param queue The log queue.
//  * @param level The log level of the message.
//  * @param message The log message.
//  * @return 0 on success, -1 if the queue is full.
//  */
// bool log_queue_push(LogQueue_T *queue, const LogEntry_T *entry) {
// bool log_queue_push(LogQueue_T *queue, LogLevel level, const char *message);
void log_queue_push(LogQueue_T *log_queue, LogLevel level, const char *log_buffer);

/**
 * @brief Pops a log entry from the log queue.
 * @param queue The log queue.
 * @param entry The log entry to populate.
 * @return 0 on success, -1 if the queue is empty.
 */
int log_queue_pop(LogQueue_T *queue, LogEntry_T *entry);

#endif // LOG_QUEUE_H
