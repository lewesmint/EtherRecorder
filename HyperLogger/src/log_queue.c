#include "log_queue.h"
#include "logger.h"
//#include "platform_mutex.h"
//
//#include <string.h>
//#include <stdio.h>
#include <windows.h> // Include Windows API for atomic operations

//#include "platform_threads.h"
//#include "platform_utils.h"
//#include "app_thread.h" // Include the app_thread header

LogQueue_T global_log_queue; // Define the log queue

/**
 * @copydoc log_queue_init
 */
void log_queue_init(LogQueue_T *queue) {
    queue->head = 0;
    queue->tail = 0;  
}

bool log_queue_push(LogQueue_T *log_queue, const LogEntry_T *entry) {
    if (!entry) {
        return false; // Prevent null pointer access
    }
    if (entry->thread_label == NULL) {
        return false; // Prevent logging of invalid thread label
    }
    long head = log_queue->head;
    long next_head = (head + 1) % LOG_QUEUE_SIZE;

    // Check if the queue is full
    if (next_head == log_queue->tail) {
        // calling logger_log would be recursive, and lead to probable stack overflow

        LogEntry_T entry;
        create_log_entry(&entry, LOG_ERROR, "Log queue overflow. Discarding oldest log entry.");
        log_now(&entry);
        InterlockedExchange(&log_queue->tail, (log_queue->tail + 1) % LOG_QUEUE_SIZE); // Discard oldest entry
    }

    // Copy the pre-constructed entry into the queue
    log_queue->entries[head] = *entry; // Struct assignment (safe and efficient)

    // Atomically update the head index
    InterlockedExchange(&log_queue->head, next_head);

    return true; // Successfully added log entry
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


