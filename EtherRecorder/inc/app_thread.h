#ifndef APP_THREAD_H
#define APP_THREAD_H

#include "platform_threads.h"
#include "logger.h"

typedef void* (*PreCreateFunc_T)(void* arg);
typedef void* (*PostCreateFunc_T)(void* arg);
typedef void* (*InitFunc_T)(void* arg);
typedef void* (*ExitFunc_T)(void* arg);


typedef struct AppThreadArgs_T {
    const char *label;                  // e.g., "CLIENT" or "SERVER"
    ThreadFunc_T func;                  // Actual function to execute
    PlatformThread_T thread_id;         // Thread ID
    void *data;                         // Thread-specific data
    PreCreateFunc_T pre_create_func;    // Pre-create function
    PostCreateFunc_T post_create_func;  // Post-create function
    InitFunc_T init_func;               // Initialization function
    ExitFunc_T exit_func;               // Exit function
} AppThreadArgs_T;

// // Define number of threads
// #define NUM_THREADS 6

// Declare the array (extern means it's defined in another source file)
extern AppThreadArgs_T all_threads[];

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
 */
void start_threads();

// /**
//  * @brief Sets the label of the current thread.
//  * @param label The label of the thread.
//  */
// void set_thread_label(const char *label);

const char* get_thread_label();

#endif // APP_THREAD_H
