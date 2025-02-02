#include "log_queue.h"

#include <string.h>
#include <stdio.h>
#include <windows.h> // Include Windows API for atomic operations

#include "platform_threads.h"
#include "platform_utils.h"
#include "logger.h" // Include the logger header
#include "app_thread.h" // Include the app_thread header

LogQueue_T log_queue; // Define the log queue

/**
 * @copydoc log_queue_init
 */
void log_queue_init(LogQueue_T *queue) {
    queue->head = 0;
    queue->tail = 0;
}

void log_queue_push(LogQueue_T *log_queue, LogLevel level, const char *log_buffer) {
    LONG head = log_queue->head;
    LONG next_head = (head + 1) % LOG_QUEUE_SIZE;

    if (next_head == log_queue->tail) {
        // Queue is full
        return;
    }

    log_queue->entries[head].level = level;
    strncpy(log_queue->entries[head].message, log_buffer, LOG_MSG_BUFFER_SIZE);

    // Atomically update the head index
    InterlockedExchange(&log_queue->head, next_head);
}

/**
 * @copydoc log_queue_pop
 */
int log_queue_pop(LogQueue_T *queue, LogEntry_T *entry) {
    LONG tail = queue->tail;

    if (tail == queue->head) {
        // Queue is empty
        return -1;
    }

    *entry = queue->entries[tail];

    // Atomically update the tail index
    InterlockedExchange(&queue->tail, (tail + 1) % LOG_QUEUE_SIZE);
    return 0;
}
