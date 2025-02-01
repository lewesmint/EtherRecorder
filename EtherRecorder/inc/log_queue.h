#ifndef LOG_QUEUE_H
#define LOG_QUEUE_H

#include <stdatomic.h>
#include <stddef.h>

#include "logger.h"

#define LOG_QUEUE_SIZE 1024 // Size of the log queue
#define THREAD_LABEL_SIZE 64 // Buffer size for thread labels

typedef struct LogEntry_T {
    LogLevel level;
    char message[LOG_MSG_BUFFER_SIZE];
    char thread_label[THREAD_LABEL_SIZE]; // Add thread label field
} LogEntry_T;

typedef struct LogQueue_T {
    atomic_size_t head;
    atomic_size_t tail;
    LogEntry_T entries[LOG_QUEUE_SIZE];
} LogQueue_T;

extern LogQueue_T log_queue; // Declare the log queue

void log_queue_init(LogQueue_T *queue);
int log_queue_push(LogQueue_T *queue, LogLevel level, const char *message);
int log_queue_pop(LogQueue_T *queue, LogEntry_T *entry);

// void* log_thread_function(void* arg);
// void* log_thread_function_impl(void* arg); // Declare the new function

#endif // LOG_QUEUE_H
