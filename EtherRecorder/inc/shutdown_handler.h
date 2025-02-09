#ifndef SHUTDOWN_HANDLER_H
#define SHUTDOWN_HANDLER_H

#include <stdbool.h>

//extern CONDITION_VARIABLE shutdown_condition;
//extern CRITICAL_SECTION shutdown_mutex;
//
//void request_shutdown(void);

bool wait_for_shutdown_signal(int timeout_ms);

/**
 * @brief Installs the signal handler for SIGINT (Ctrl?C).
 *
 * This initialises the shutdown mechanism so that when a SIGINT is received,
 * all threads can check the shutdown flag and terminate gracefully.
 */
void install_shutdown_handler(void);

/**
 * @brief Called by the signal handler to request shutdown.
 *
 * This sets an internal flag that indicates that a shutdown has been requested.
 */
void shutdown_requested(void);

/**
 * @brief Checks whether a shutdown has been requested.
 *
 * @return Non?zero if shutdown is requested, zero otherwise.
 */
bool is_shutdown_requested(void);

void request_shutdown(void);

bool wait_for_shutdown_signal(int timeout_ms);



#endif // SHUTDOWN_HANDLER_H