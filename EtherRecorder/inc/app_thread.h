/**
* @file app_thread.h
* @brief Thread management functions.
*
*This file contains the functions to manage threads.
*
*/

#ifndef APP_THREAD_H
#define APP_THREAD_H

#include "platform_threads.h"
#include "logger.h"
#include <stdbool.h>
#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif


extern CONDITION_VARIABLE shutdown_condition;
extern CRITICAL_SECTION shutdown_mutex;

void request_shutdown(void);
bool wait_for_shutdown(int timeout_ms);

/**
 * @brief Function pointer type for pre-create functions.
 * @param arg Argument to the function.
 * @return Pointer to the result.
 */
typedef void* (*PreCreateFunc_T)(void* arg);

/**
 * @brief Function pointer type for post-create functions.
 * @param arg Argument to the function.
 * @return Pointer to the result.
 */
typedef void* (*PostCreateFunc_T)(void* arg);

/**
 * @brief Function pointer type for initialisation functions.
 * @param arg Argument to the function.
 * @return Pointer to the result.
 */
typedef void* (*InitFunc_T)(void* arg);

/**
 * @brief Function pointer type for exit functions.
 * @param arg Argument to the function.
 * @return Pointer to the result.
 */
typedef void* (*ExitFunc_T)(void* arg);

/**
 * @brief Structure to hold thread arguments and functions.
 */
typedef struct AppThreadArgs_T {
    const char label[THREAD_LABEL_SIZE]; ///< Label for the thread (e.g., "CLIENT" or "SERVER")
    ThreadFunc_T func;                   ///< Actual function to execute
    PlatformThread_T thread_id;          ///< Thread ID
    void *data;                          ///< Thread-specific data
    PreCreateFunc_T pre_create_func;     ///< Pre-create function
    PostCreateFunc_T post_create_func;   ///< Post-create function
    InitFunc_T init_func;                ///< Initialisation function
    ExitFunc_T exit_func;                ///< Exit function
} AppThreadArgs_T;

/**
 * @brief Starts the threads based on the thread table.
 * @param threads The thread table.
 */
void start_threads();

/**
 * @brief Gets the label of the current thread.
 * @return The label of the current thread.
 */
const char* get_thread_label();

/**
 * @brief Sets the label of the current thread.
 * @param label The label to set.
 */
void set_thread_label(const char *label);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // APP_THREAD_H
