#include "log_queue.h"
#include "logger.h"
#include "platform_mutex.h"

#include <string.h>
#include <stdio.h>
#include <windows.h> // Include Windows API for atomic operations

#include "platform_threads.h"
#include "platform_utils.h"
#include "app_thread.h" // Include the app_thread header

LogQueue_T log_queue; // Define the log queue

/**
 * @copydoc log_queue_init
 */
void log_queue_init(LogQueue_T *queue) {
    queue->head = 0;
    queue->tail = 0;  
}

bool log_queue_push(LogQueue_T *log_queue, LogLevel level, const char *log_buffer) {

    printf("log_queue_push\n");

    LONG head = log_queue->head;
    LONG next_head = (head + 1) % LOG_QUEUE_SIZE;

    if (next_head == log_queue->tail) {
        // Queue is full, handle overflow
        logger_log(LOG_WARN, "Log queue overflow. Discarding oldest log entry.");
        log_queue->tail = (log_queue->tail + 1) % LOG_QUEUE_SIZE; // Discard the oldest log entry
        return false; // Indicate that the queue is full
    }

    log_queue->entries[head].level = level;
    strncpy(log_queue->entries[head].message, log_buffer, LOG_MSG_BUFFER_SIZE);

    // Atomically update the head index
    InterlockedExchange(&log_queue->head, next_head);
    return true; // Indicate success
}

/**
 * @copydoc log_queue_pop
 */
bool log_queue_pop(LogQueue_T *queue, LogEntry_T *entry) {
    LONG tail = queue->tail;

    if (tail == queue->head) {
        // Queue is empty
        return false;
    }

    *entry = queue->entries[tail];

    // Atomically update the tail index
    InterlockedExchange(&queue->tail, (tail + 1) % LOG_QUEUE_SIZE);
    return true;
}

bool log_queue_pop_debug(LogQueue_T *queue, LogEntry_T *entry) {
    LONG tail = queue->tail;

    printf("log_queue_pop_debug (deliberately slow) Queue size: %ld, head: %ld, tail: %ld\n", queue->head - tail, queue->head, tail);

    if (tail == queue->head) {
        // Queue is empty
        return false;
    }

    *entry = queue->entries[tail];

    // Atomically update the tail index
    InterlockedExchange(&queue->tail, (tail + 1) % LOG_QUEUE_SIZE);
    return true;
}

