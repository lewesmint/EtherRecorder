#include "log_queue.h"
#include "platform_threads.h"
#include "platform_utils.h"
#include "logger.h" // Include the logger header
#include "app_thread.h" // Include the app_thread header
#include <string.h>
#include <stdio.h>

log_queue_t log_queue; // Define the log queue

void log_queue_init(log_queue_t *queue) {
    atomic_init(&queue->head, 0);
    atomic_init(&queue->tail, 0);
}

int log_queue_push(log_queue_t *queue, LogLevel level, const char *message) {
    size_t head = atomic_load(&queue->head);
    size_t next_head = (head + 1) % LOG_QUEUE_SIZE;

    if (next_head == atomic_load(&queue->tail)) {
        // Queue is full
        return -1;
    }

    queue->entries[head].level = level;
    strncpy(queue->entries[head].message, message, LOG_BUFFER_SIZE - 1);
    queue->entries[head].message[LOG_BUFFER_SIZE - 1] = '\0';
    strncpy(queue->entries[head].thread_label, get_thread_label(), THREAD_LABEL_SIZE - 1);
    queue->entries[head].thread_label[THREAD_LABEL_SIZE - 1] = '\0';

    atomic_store(&queue->head, next_head);
    return 0;
}

int log_queue_pop(log_queue_t *queue, log_entry_t *entry) {
    size_t tail = atomic_load(&queue->tail);

    if (tail == atomic_load(&queue->head)) {
        // Queue is empty
        return -1;
    }

    *entry = queue->entries[tail];
    atomic_store(&queue->tail, (tail + 1) % LOG_QUEUE_SIZE);
    return 0;
}

// void* log_thread_function(void* arg) {
//     return launch_thread_function(log_thread_function_impl, arg, "log");
// }

void* log_thread_function_impl(void* arg) {
    (void)arg;
    log_entry_t entry;

    while (1) {
        while (log_queue_pop(&log_queue, &entry) == 0) {
            // Log using the logger function
            log_immediately(entry.message);
        }
        platform_sleep(10); // Sleep for a short while before checking the queue again
    }

    return NULL;
}
