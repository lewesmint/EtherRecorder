#include "shutdown_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "logger.h"

/* A flag to indicate that a shutdown has been requested.
 * Marked as volatile because it is modified asynchronously in the control handler.
 */
volatile bool shutdown_flag = false;

// Global shutdown event handle.
static HANDLE shutdown_event = NULL;


/**
 * @brief Windows console control handler.
 *
 * This function is called by the system when a console event (such as Ctrl?C)
 * is received. When a CTRL_C_EVENT occurs, the shutdown flag is set.
 *
 * @param signal The type of console control event.
 * @return TRUE if the signal was handled, FALSE otherwise.
 */
BOOL WINAPI console_ctrl_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        fprintf(stderr, "\nCTRL-C detected. Initiating shutdown...\n");
        shutdown_flag = TRUE;
        if (shutdown_event != NULL) {
            if (!SetEvent(shutdown_event)) {
                fprintf(stderr, "SetEvent failed with error %lu\n", GetLastError());
            }
        }
        return TRUE;  // Signal was handled
    }
    return FALSE;  // Let other handlers process this event.
}


/**
 * @brief Installs the console control handler.
 *
 * This should be called once application's initialisation (e.g. in app_init()).
 */
void install_shutdown_handler(void) {

    // Create a manual-reset event that is initially non-signaled.
    shutdown_event = CreateEvent(NULL,   // Default security attributes.
        TRUE,   // Manual-reset event.
        FALSE,  // Initial state is non-signaled.
        NULL);  // No name.

    if (shutdown_event == NULL) {
        logger_log(LOG_ERROR, "CreateEvent failed (%lu)", GetLastError());
        exit(EXIT_FAILURE);
    }

    // Register the console control handler.
    if (!SetConsoleCtrlHandler(console_ctrl_handler, TRUE)) {
        logger_log(LOG_ERROR, "Could not set control handler");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Returns the current state of the shutdown flag.
 *
 */
bool is_shutdown_requested(void) {
    return shutdown_flag;
}


void request_shutdown(void) {
    //EnterCriticalSection(&shutdown_mutex);
    shutdown_flag = true;
    //WakeAllConditionVariable(&shutdown_condition);
    //LeaveCriticalSection(&shutdown_mutex);
}

/**
 * @brief Block until the shutdown event is signaled, or the timeout expires.
 */
bool wait_for_shutdown_signal(int timeout_ms) {
	// Note INFINITE (-1) is a possibility for timeout_ms.    
    DWORD result = WaitForSingleObject(shutdown_event, timeout_ms);
    if (result == WAIT_OBJECT_0) {
        return true;
    }
    else if (result == WAIT_TIMEOUT) {
        logger_log(LOG_ERROR, "Wait for Shutdown timeout (%d ms) exceeded", timeout_ms);
    }
    else if (result == WAIT_ABANDONED) {
        logger_log(LOG_ERROR, "Wait for Shutdown was abandoned");
    }
    else if (result == WAIT_FAILED) {
		DWORD error_code = GetLastError();
		// TODO Convert error_code to string
		logger_log(LOG_ERROR, "Wait for Shutdown to be signalled failed on %h", error_code);
	}
    else {
		logger_log(LOG_ERROR, "Wait for Shutdown failed for unknown reason");
	}
	return false;
}

