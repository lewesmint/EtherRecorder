#ifndef LOG_QUEUE_H
#define LOG_QUEUE_H

#include "logger.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_QUEUE_SIZE 1024 // Size of the log queue


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


bool log_queue_push(LogQueue_T *log_queue, const LogEntry_T *entry);

/**
 * @brief Pops a log entry from the log queue.
 * @param queue The log queue.
 * @param entry The log entry to populate.
 * @return 0 on success, -1 if the queue is empty.
 */
bool log_queue_pop(LogQueue_T *queue, LogEntry_T *entry);

bool log_queue_pop_debug(LogQueue_T *queue, LogEntry_T *entry);

#ifdef __cplusplus
}   // extern "C"
#endif


#endif // LOG_QUEUE_H
