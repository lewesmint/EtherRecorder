#include "client_manager.h"

#include <stdio.h> // for perror

#include "common_socket.h"
#include "app_thread.h"
#include "logger.h"
#include "platform_utils.h"


extern volatile bool shutdown_flag;

// void* clientMainThread(void* arg) {
//     AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
//     set_thread_label(thread_info->label);
//     ClientThreadArgs_T* client_info = (ClientThreadArgs_T*)thread_info->data;

//     int port = client_info->port;
//     bool is_tcp = client_info->is_tcp;
//     bool is_server = false;

//     // TODO refactor into a common routine in common_socket 
//     /* Prepare socket addresses */
//     struct sockaddr_in addr, client_addr;
//     memset(&addr, 0, sizeof(addr));
//     memset(&client_addr, 0, sizeof(client_addr));
//     socklen_t client_len = sizeof(client_addr);

//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(port);

//     // If server: bind/listen on 0.0.0.0 (INADDR_ANY)
//     // If client: connect to supplied port 
//     const char *host = client_info->client_hostname;
//     if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
//         perror("inet_pton failed");
//         return NULL;
//     }

//     /* Create and set up the socket */
//     SOCKET sock = setup_socket(is_server, is_tcp, &addr, &client_addr, host, port);
//     if (sock == INVALID_SOCKET) {
//         print_socket_error("Socket setup failed");
//         return NULL; 
//     }

//     logger_log(LOG_INFO, "CLIENT: Connecting to %s on port %d", host, port);

//     bool reconnection_required = true;
//     int backoff = 1;  // Start backoff at 1 second

//     while (reconnection_required && !shutdown_flag) {  // ✅ Check shutdown flag
//         if (is_tcp) {
//             int connection_result = connect_with_timeout(sock, &addr, 5);  // 5s timeout

//             if (connection_result == 0) {
//                 logger_log(LOG_INFO, "Connected to server successfully.");
//                 reconnection_required = false;
//             } else {
//                 logger_log(LOG_ERROR, "Connection failed. Retrying in %d seconds...", backoff);

//                 // Close the existing socket before retrying
//                 close_socket(&sock);

//                 // **Check shutdown flag before retrying**
//                 if (shutdown_flag) {
//                     logger_log(LOG_INFO, "Shutdown requested. Exiting client thread.");
//                     return NULL;
//                 }

//                 // Exponential backoff (max 32s)
//                 sleep(backoff);
//                 backoff = (backoff < 32) ? backoff * 2 : 32;

//                 // **Recreate the socket before retrying**
//                 sock = setup_socket(is_server, is_tcp, &addr, &client_addr, host, port);
//                 if (sock == INVALID_SOCKET) {
//                     print_socket_error("Socket setup failed during reconnection");
//                     return NULL; //
//                 }
//             }
//         } else {
//             // UDP client logic - nothing needed for reconnection
//             logger_log(LOG_INFO, "UDP Client ready to send on port %d.", port);
//             reconnection_required = false;
//         }
//     }

//     // **Final shutdown check before entering communication loop**
//     if (shutdown_flag) {
//         logger_log(LOG_INFO, "Shutdown requested before communication started.");
//         close_socket(&sock);
//         return NULL;
//     }

//     // Now we can do the communication loop
//     communication_loop(sock, is_server, is_tcp, &client_addr);

//     // Ensure socket is closed before exiting
//     close_socket(&sock);

//     return NULL;
// }

void* clientMainThread(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    set_thread_label(thread_info->label);
    ClientThreadArgs_T* client_info = (ClientThreadArgs_T*)thread_info->data;

    int port = client_info->port;
    bool is_tcp = client_info->is_tcp;
    bool is_server = false;
    SOCKET sock = INVALID_SOCKET;

    struct sockaddr_in addr, client_addr;
    memset(&addr, 0, sizeof(addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // logger_log(LOG_INFO, "CLIENT: Resolving hostname %s...", client_info->client_hostname);

    bool reconnection_required = true;
    int backoff = 1;  // Start backoff at 1 second

    while (reconnection_required && !shutdown_flag) {
        /// setup socket handles everything required for the client to connect to the server
        sock = setup_socket(is_server, is_tcp, &addr, &client_addr, client_info->client_hostname, port);
        if (sock == INVALID_SOCKET) {
            logger_log(LOG_ERROR, "Socket setup failed. Retrying in %d seconds...", backoff);
            sleep(backoff);
            backoff = (backoff < 32) ? backoff * 2 : 32;  // Exponential backoff
            continue;  // Retry setup
        }

        if (is_tcp) {
            if (connect_with_timeout(sock, &addr, 5) == 0) {  // 5s timeout
                logger_log(LOG_INFO, "Client Mananger connected to server.");
                reconnection_required = false;
            } else {
                logger_log(LOG_ERROR, "Connection failed. Retrying in %d seconds...", backoff);
                close_socket(&sock);
                sleep(backoff);
                backoff = (backoff < 32) ? backoff * 2 : 32;
                continue;
            }
        } else {
            // UDP clients don’t need to connect
            logger_log(LOG_INFO, "UDP Client ready to send on port %d.", port);
            reconnection_required = false;
        }
    }

    if (shutdown_flag) {
        logger_log(LOG_INFO, "Shutdown requested before communication started.");
        close_socket(&sock);
        return NULL;
    }

    // Now we can do the communication loop
    // eventually, this will be moved to a  two deiicated threads for sending and receiving
    communication_loop(sock, is_server, is_tcp, &client_addr);
    close_socket(&sock);

    logger_log(LOG_INFO, "CLIENT: Exiting client thread.");
    return NULL;
}

