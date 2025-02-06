#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <stdbool.h>

/**
 * @brief Structure to hold servr maanager thread arguments and functions.
 */
typedef struct ClientThreadArgs_T {
    // const char *label;                  ///< Label for the thread (e.g., "CLIENT" or "SERVER")
    // ThreadFunc_T func;                  ///< Actual function to execute
    // PlatformThread_T thread_id;         ///< Thread ID
    void *data;                          ///< Server Thread-specific data
    // PreCreateFunc_T pre_create_func;    ///< Pre-create function
    // PostCreateFunc_T post_create_func;  ///< Post-create function
    // InitFunc_T init_func;               ///< Initialization function
    // ExitFunc_T exit_func;               ///< Exit function
    char *client_hostname;                 ///< Client hostname or IP address
    int port;                            ///< Port number
    bool is_tcp;                         ///< Procoeol is TCP (else UDP)
} ClientThreadArgs_T;

void* clientMainThread(void* arg);

#endif // CLIENT_MANAGER_H