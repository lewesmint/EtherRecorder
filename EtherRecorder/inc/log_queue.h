#ifndef LOG_QUEUE_H
#define LOG_QUEUE_H

#include <stdatomic.h>
#include <stddef.h>
#include "logger.h"

#define LOG_QUEUE_SIZE 1024
#define LOG_BUFFER_SIZE 1024 // Buffer size for log messages
#define THREAD_LABEL_SIZE 64 // Buffer size for thread labels

typedef struct {
    LogLevel level;
    char message[LOG_BUFFER_SIZE];
    char thread_label[THREAD_LABEL_SIZE]; // Add thread label field
} log_entry_t;

typedef struct {
    atomic_size_t head;
    atomic_size_t tail;
    log_entry_t entries[LOG_QUEUE_SIZE];
} log_queue_t;

extern log_queue_t log_queue; // Declare the log queue

void log_queue_init(log_queue_t *queue);
int log_queue_push(log_queue_t *queue, LogLevel level, const char *message);
int log_queue_pop(log_queue_t *queue, log_entry_t *entry);

// void* log_thread_function(void* arg);
// void* log_thread_function_impl(void* arg); // Declare the new function

#endif // LOG_QUEUE_H
