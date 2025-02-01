#ifndef APP_THREAD_H
#define APP_THREAD_H

#include "platform_threads.h"
#include "logger.h"

typedef void* (*pre_create_func_t)(void* arg);
typedef void* (*post_create_func_t)(void* arg);
typedef void* (*init_func)(void* arg);
typedef void* (*exit_func)(void* arg);


typedef struct _app_thread_args_t {
    const char *label;                   // e.g., "CLIENT" or "SERVER"
    thread_func_t func;                  // Actual function to execute
    platform_thread_t thread_id;         // Thread ID
    void *data;                          // Thread-specific data
    pre_create_func_t pre_create_func;   // Pre-create function
    post_create_func_t post_create_func; // Post-create function
    pre_create_func_t init_func;         // 
    post_create_func_t exit_func;        // 
} app_thread_args_t;


// /**
//  * @brief Template for post-initialization functions.
//  * @param thread_name The name of the thread.
//  */
// typedef void (*post_init_func_t)(const char *thread_name);

// /**
//  * @brief Thread table entry structure.
//  */
// typedef struct {
//     const char *label;
//     thread_func_t func;
//     post_init_func_t post_init_func; // Add post-initialization function
//     void *arg;
// } thread_entry_t;

// /**
//  * @brief Initializes the thread settings based on the configuration.
//  * @param thread_name The name of the thread.
//  */
// void initialize_thread_settings(const char *thread_name);

// /**
//  * @brief Launches the thread function after initializing settings.
//  * @param func The actual thread function to execute.
//  * @param arg The argument passed to the thread function.
//  * @param thread_name The name of the thread.
//  * @return The return value of the thread function.
//  */
// void* launch_thread_function(thread_func_t func, void* arg, const char *thread_name);

// /**
//  * @brief Pre-launch initialization for a thread in the context of the thread creator.
//  * @param func The actual thread function to execute.
//  * @param arg The argument passed to the thread function.
//  * @param thread_name The name of the thread.
//  * @return The return value of the thread function.
//  */
// void* prelaunch_thread_function(thread_func_t func, void* arg, const char *thread_name);

/**
 * @brief Starts the threads based on the thread table.
 * @param threads The thread table.
 * @param num_threads The number of threads in the table.
 */
void start_threads(app_thread_args_t *threads, int num_threads);

/**
 * @brief Sets the label of the current thread.
 * @param label The label of the thread.
 */
void set_thread_label(const char *label);

const char* get_thread_label();

#endif // APP_THREAD_H
