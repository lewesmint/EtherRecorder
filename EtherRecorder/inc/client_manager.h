/**
* @file client_manager.h
* @brief Client manager thread functions.
* 
*/
#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <stdbool.h>

/**
 * @brief Structure to hold server maanager thread arguments and functions.
 */
typedef struct ClientThreadArgs_T {
    void *data;                          ///< Server Thread-specific data
    int data_size;                       ///< Data size
    bool send_test_data;                 ///< Send test data
    int send_interval_ms;                ///< Send interval
    char server_hostname[100];           ///< Server hostname or IP address
    int port;                            ///< Port number
    bool is_tcp;                         ///< Protocol is TCP (else UDP)
} ClientThreadArgs_T;

void* clientMainThread(void* arg);

#endif // CLIENT_MANAGER_H