#include "client_manager.h"

#include <stdio.h>      // for perror, snprintf
#include <string.h>     // for memcpy, memset, strncpy, memmove, strerror
#include <time.h>       // for time_t, time, ctime

#include "common_socket.h"
#include "app_thread.h"
#include "logger.h"
#include "platform_utils.h"
#include "app_config.h"

// External declarations for stub functions
extern void* pre_create_stub(void* arg);
extern void* post_create_stub(void* arg);
extern void* init_stub(void* arg);
extern void* exit_stub(void* arg);
extern void* init_wait_for_logger(void* arg);
extern bool shutdown_signalled(void);

// TODO consider using a dynamic buffer, adjusting to conditions
#define BUFFER_SIZE               8192
#define SOCKET_ERROR_BUFFER_SIZE  256
#define BLOCKING_TIMEOUT_SEC 10  // Blocking timeout in seconds

static bool suppress_client_send_data = true;

typedef struct {
    int cols;   // Number of columns (each column holds 4 bytes)
    int start;  // How many bytes (of the current row) have already been filled
} config_t;

/* Assume this config is read from a configuration file earlier. */
static config_t config = { .cols = 4, .start = 0 };

#define BLOCK_SIZE 4                      // Each column holds 4 bytes.
#define BLOCK_CHAR_COUNT (BLOCK_SIZE * 2) // 4 bytes → 8 characters.
#define COL_WIDTH (BLOCK_CHAR_COUNT + 1)  // Plus one space separator.

/* 
 * Initializes the output row with the placeholder pattern.
 * Each byte is expected to be rendered as two characters, so we fill each
 * column with "........" (8 dots) followed by a space.
 */
static void init_row(char *row, int cols) {
    int total_chars = cols * COL_WIDTH;
    for (int col = 0; col < cols; col++) {
        int base = col * COL_WIDTH;
        for (int i = 0; i < BLOCK_CHAR_COUNT; i++) {
            row[base + i] = '.';
        }
        row[base + BLOCK_CHAR_COUNT] = ' ';  // trailing space for the column.
    }
    row[cols * COL_WIDTH] = '\0';
}

/*
 * For each call, we “profile” an output row – prefilled with dots – then overlay the new
 * bytes into their proper positions. The position in the row (in bytes) is determined by
 * config.start. When a row is complete, or when the new data is exhausted, we log the row.
 *
 * The mapping is as follows:
 *   Each row has a capacity of (config.cols * BLOCK_SIZE) bytes.
 *   For a byte to be placed at position 'pos' (0-based) within the row:
 *     - The column index is pos / BLOCK_SIZE.
 *     - The offset within that column is pos % BLOCK_SIZE.
 *     - The character position in the row is:
 *           col * COL_WIDTH + (offset * 2)
 *     (Each byte occupies two characters.)
 */
static void log_buffered_data(uint8_t *buffer, int *buffered_length, int batch_bytes) {
    int total = *buffered_length;
    int index = 0;
    int row_capacity = config.cols * BLOCK_SIZE;  // Number of bytes per output row.
    const char hex_chars[] = "0123456789ABCDEF";
    char row[256];  // Assume this is enough for our configured number of columns.
    
    logger_log(LOG_INFO, "%d bytes received: top", batch_bytes);
    
    /* Process the new bytes, possibly spanning multiple rows. */
    while (index < total) {
        init_row(row, config.cols);
        int avail = row_capacity - config.start;      // How many bytes can we fill in this row.
        int to_place = (total - index < avail) ? (total - index) : avail;
        
        /* Place new bytes into the profiled row. */
        for (int i = 0; i < to_place; i++) {
            int pos = config.start + i;  // Byte position within the row.
            int col = pos / BLOCK_SIZE;
            int offset = pos % BLOCK_SIZE;
            int dest_index = col * COL_WIDTH + offset * 2;
            uint8_t byte = buffer[index + i];
            row[dest_index]     = hex_chars[byte >> 4];
            row[dest_index + 1] = hex_chars[byte & 0x0F];
        }
        
        config.start += to_place;
        if (config.start >= row_capacity) {
            config.start = 0;  // Start a fresh row on the next iteration.
        }
        index += to_place;
        
        logger_log(LOG_INFO, "%s", row);
    }
    
    *buffered_length = 0;
    logger_log(LOG_INFO, "%d bytes received: bottom", batch_bytes);
}

/**
 * Attempts to set up the socket connection, including retries with
 * exponential backoff. For TCP, it tries to connect with a 5-second timeout.
 * For UDP, no connection attempt is necessary.
 *
 * @param is_server      Flag indicating if this is a server.
 * @param is_tcp         Flag indicating if the socket is TCP.
 * @param addr           Pointer to the sockaddr_in for the server address.
 * @param client_addr    Pointer to the sockaddr_in for the client address.
 * @param hostname       The server hostname.
 * @param port           The port number.
 * @return A valid socket on success, or INVALID_SOCKET on failure.
 */
static SOCKET attempt_connection(bool is_server, bool is_tcp, struct sockaddr_in* addr,
    struct sockaddr_in* client_addr, const char* hostname, int port) {
    int backoff = 1;  // Start with a 1-second backoff.
    while (!shutdown_signalled()) {
        logger_log(LOG_DEBUG, "Client Manager Attempting to connect to server %s on port %d...", hostname, port);
        SOCKET sock = setup_socket(is_server, is_tcp, addr, client_addr, hostname, port);
        if (sock == INVALID_SOCKET) {
            logger_log(LOG_ERROR, "Socket setup failed. Retrying in %d seconds...", backoff);
            {
                char error_buffer[SOCKET_ERROR_BUFFER_SIZE];
                get_socket_error_message(error_buffer, sizeof(error_buffer));
                logger_log(LOG_ERROR, "%s", error_buffer);
            }
            logger_log(LOG_DEBUG, "Client Manager Sleeping");
            sleep(backoff);
            logger_log(LOG_DEBUG, "Client Manager Waking");

            backoff = (backoff < 32) ? backoff * 2 : 32;
            continue;
        }

        if (is_tcp) {
            if (connect_with_timeout(sock, addr, 5) == 0) {  // 5-second timeout.
                logger_log(LOG_INFO, "Client Manager connected to server.");
                return sock;
            }
            else {
                logger_log(LOG_ERROR, "Connection failed. Retrying in %d seconds...", backoff);
                {
                    char error_buffer[SOCKET_ERROR_BUFFER_SIZE];
                    get_socket_error_message(error_buffer, sizeof(error_buffer));
                    logger_log(LOG_ERROR, "%s", error_buffer);
                }
                if (sock != INVALID_SOCKET) {
                    close_socket(&sock);
                }
                sleep(backoff);
                backoff = (backoff < 32) ? backoff * 2 : 32;
                continue;
            }
        }
        else {
            // For UDP clients no connection attempt is required.
            logger_log(LOG_INFO, "UDP Client ready to send on port %d.", port);
            return sock;
        }
    }
    while (!shutdown_signalled()) {
        logger_log(LOG_INFO, "Client Manager attempt to connect exiting due to app shutdown.");
    }
    return INVALID_SOCKET;
}

/*
 * handle_data_reception() reads data from the socket, updating both the overall
 * buffered length and the count of new bytes (batch_bytes) since the last log.
 */
static bool handle_data_reception(SOCKET sock, char* buffer, int* buffered_length, int* batch_bytes) {
    // only read after a select, so we know data is available
    int buffer_available = BUFFER_SIZE - *buffered_length;
    if (buffer_available < 0) {
        logger_log(LOG_ERROR, "Buffer underflow detected! Available bytes: %d", buffer_available);
    } else {
        logger_log(LOG_DEBUG, "buffer available =- %d", buffer_available);
    }
    int bytes = recv(sock, buffer + *buffered_length, BUFFER_SIZE - *buffered_length, 0);
    if (bytes <= 0) {
        char err_buf[SOCKET_ERROR_BUFFER_SIZE];
        get_socket_error_message(err_buf, sizeof(err_buf));
        logger_log(LOG_ERROR, "recv error or connection closed (received %d bytes): %s", bytes, err_buf);
        return false;
    }
    *buffered_length += bytes;
    *batch_bytes += bytes;
    logger_log(LOG_DEBUG, "Received %d bytes", bytes);
    return true;
}

/**
 * @brief Sets up the fd_set and timeval for select().
 *
 * This helper function clears the fd_set, adds the given socket, and sets the timeout.
 *
 * @param sock     The socket descriptor to monitor.
 * @param read_fds Pointer to the fd_set to be used with select().
 * @param timeout  Pointer to the timeval structure to configure.
 * @param sec      Seconds part of the timeout.
 * @param usec     Microseconds part of the timeout.
 */
void setup_select_timeout(SOCKET* sock, fd_set* write_fds, struct timeval* timeout, long sec, long usec) {
    FD_ZERO(write_fds);
    if (sock != NULL && *sock != INVALID_SOCKET) {
        FD_SET(*sock, write_fds);
    }
    else {
        logger_log(LOG_ERROR, "Invalid socket before select()!");
    }
    timeout->tv_sec = sec;
    timeout->tv_usec = usec;
}

typedef struct {
    SOCKET* sock;  // Pointer to shared socket
    struct sockaddr_in client_addr;
    ClientThreadArgs_T* client_info;
    volatile bool connection_closed;  // Shared flag to indicate socket closure
} ClientCommArgs_T;


void* client_receive_thread(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    set_thread_label(thread_info->label);
    ClientCommArgs_T* comm_args = (ClientCommArgs_T*)thread_info->data;
    SOCKET* sock = comm_args->sock;
    ClientThreadArgs_T* client_info = comm_args->client_info;

    char buffer[BUFFER_SIZE];
    int buffered_length = 0;
    int batch_bytes = 0;

    if (*sock == INVALID_SOCKET) {
        logger_log(LOG_ERROR, "Invalid socket. Exiting receive thread.");
        return NULL;
    }

    while (!shutdown_signalled() && !comm_args->connection_closed) {
        fd_set read_fds;
        struct timeval timeout;

        setup_select_timeout(sock, &read_fds, &timeout, BLOCKING_TIMEOUT_SEC, 0);
        int ret = select((int)(*sock) + 1, &read_fds, NULL, NULL, &timeout);
        if (ret > 0 && FD_ISSET(*sock, &read_fds)) {
            if (!handle_data_reception(*sock, buffer, &buffered_length, &batch_bytes)) {
                logger_log(LOG_ERROR, "Connection closed by server. Closing socket.");
                close_socket(sock);
                *sock = INVALID_SOCKET;
                comm_args->connection_closed = true;
                break;
            }
            if (buffered_length > 0) {
                log_buffered_data((uint8_t*)buffer, &buffered_length, batch_bytes);
                batch_bytes = 0;
            }
        } else if (ret == 0) {
            logger_log(LOG_DEBUG, "Timeout: No data received within %d seconds", BLOCKING_TIMEOUT_SEC);
        } else {
            logger_log(LOG_ERROR, "Select error in receive thread. Exiting loop.");
            break;
        }
    }

    if (*sock != INVALID_SOCKET) {
        close_socket(sock);
        *sock = INVALID_SOCKET;
    }
    comm_args->connection_closed = true;

    logger_log(LOG_INFO, "Receive thread exiting.");
    return NULL;
}

void* client_send_thread(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    set_thread_label(thread_info->label);
    ClientCommArgs_T* comm_args = (ClientCommArgs_T*)thread_info->data;
    SOCKET* sock = comm_args->sock;
    ClientThreadArgs_T* client_info = comm_args->client_info;

    while (!shutdown_signalled() && !comm_args->connection_closed) {
        if (*sock == INVALID_SOCKET) {
            logger_log(LOG_INFO, "Send thread detected socket closure. Exiting.");
            break;
        }

        if (!client_info->send_test_data || client_info->data == NULL || client_info->send_interval_ms <= 0) {
            sleep_ms(500);
            continue;
        }

        fd_set write_fds;
        struct timeval timeout;
        setup_select_timeout(sock, &write_fds, &timeout, BLOCKING_TIMEOUT_SEC, 0);
        int ret = select((int)*sock + 1, NULL, &write_fds, NULL, &timeout);

        if (ret > 0 && FD_ISSET(*sock, &write_fds)) {
            int sent = send(*sock, client_info->data, client_info->data_size, 0);
            if (sent == SOCKET_ERROR) {
                logger_log(LOG_ERROR, "Send error while sending periodic data.");
                close_socket(sock);
                *sock = INVALID_SOCKET;
                comm_args->connection_closed = true;
                break;
            } else {
                logger_log(LOG_INFO, "Periodic send: sent %d bytes", sent);
            }
            sleep_ms(client_info->send_interval_ms);
        } else if (ret == 0) {
            logger_log(LOG_DEBUG, "Timeout: No write availability within %d seconds", BLOCKING_TIMEOUT_SEC);
        } else {
            logger_log(LOG_ERROR, "Select error in send thread. Exiting loop.");
            break;
        }
    }

    logger_log(LOG_INFO, "Send thread exiting.");
    return NULL;
}

AppThreadArgs_T send_thread_args = {
	.suppressed = true,
    .label = "CLIENT.SEND",
    .func = client_send_thread,
    .data = NULL,
    .pre_create_func = pre_create_stub,
    .post_create_func = post_create_stub,
    .init_func = init_wait_for_logger,
    .exit_func = exit_stub
};

AppThreadArgs_T receive_thread_args = {
    .suppressed = true,
    .label = "CLIENT.RECEIVE",
    .func = client_receive_thread,
    .data = NULL,
    .pre_create_func = pre_create_stub,
    .post_create_func = post_create_stub,
    .init_func = init_wait_for_logger,
    .exit_func = exit_stub
};

void* clientMainThread(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    set_thread_label(thread_info->label);
    ClientThreadArgs_T* client_info = (ClientThreadArgs_T*)thread_info->data;

    const char* config_server_hostname = get_config_string("network", "client.server_hostname", NULL);
    if (config_server_hostname) {
        strncpy(client_info->server_hostname, config_server_hostname, sizeof(client_info->server_hostname));
        client_info->server_hostname[sizeof(client_info->server_hostname) - 1] = '\0';
    }
    client_info->port = get_config_int("network", "client.port", client_info->port);
    client_info->send_interval_ms = get_config_int("network", "client.send_interval_ms", client_info->send_interval_ms);
    client_info->send_test_data = get_config_bool("network", "client.send_test_data", false);
    suppress_client_send_data = get_config_bool("debug", "suppress_client_send_data", false);
    logger_log(LOG_INFO, "Client Manager will attempt to connect to Server: %s, port: %d", client_info->server_hostname, client_info->port);

    int port = client_info->port;
    bool is_tcp = client_info->is_tcp;
    bool is_server = false;
    SOCKET sock = INVALID_SOCKET;

    struct sockaddr_in addr, client_addr;

    while (!shutdown_signalled()) {
        memset(&addr, 0, sizeof(addr));
        memset(&client_addr, 0, sizeof(client_addr));

        sock = attempt_connection(is_server, is_tcp, &addr, &client_addr, client_info->server_hostname, port);
        if (sock == INVALID_SOCKET) {
            logger_log(LOG_INFO, "Shutdown requested before communication started.");
            return NULL;
        }

        ClientCommArgs_T comm_args = {&sock, client_addr, client_info };
        AppThreadArgs_T send_thread_args_local = send_thread_args;
        AppThreadArgs_T receive_thread_args_local = receive_thread_args;
        
        send_thread_args_local.data = &comm_args;
        receive_thread_args_local.data = &comm_args;
        send_thread_args_local.suppressed = false;
        receive_thread_args_local.suppressed = false;
   
        create_app_thread(&send_thread_args_local);
        create_app_thread(&receive_thread_args_local);

        // Wait for send and receive threads to complete with a timeout
        while (!shutdown_signalled()) {

            logger_log(LOG_INFO, "CLIENT: Looping on waiting for send and receive threads to indicate they're done");

            DWORD send_thread_result = WaitForSingleObject(send_thread_args_local.thread_id, 5000); // 500 ms timeout
            DWORD receive_thread_result = WaitForSingleObject(receive_thread_args_local.thread_id, 5000); // 500 ms timeout

            if ((send_thread_result == WAIT_OBJECT_0 || send_thread_result == WAIT_FAILED) &&
                (receive_thread_result == WAIT_OBJECT_0 || receive_thread_result == WAIT_FAILED)) {
                break;
            }
        }

        logger_log(LOG_INFO, "Closing socket 1");
        if (sock != INVALID_SOCKET) {
            logger_log(LOG_INFO, "Closing socket 2");
            close_socket(&sock);
        }

        while (!shutdown_signalled()) {
            logger_log(LOG_INFO, "CLIENT: Shutdown is signaled detected by client");
            break;
        }

        logger_log(LOG_INFO, "CLIENT: Connection lost. Attempting to reconnect...");
    }

    logger_log(LOG_INFO, "CLIENT: Exiting client thread.");
    return NULL;
}
