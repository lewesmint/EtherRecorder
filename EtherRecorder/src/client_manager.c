#include "client_manager.h"

#include <stdio.h>      // for perror, snprintf
#include <string.h>     // for memcpy, memset, strncpy, memmove, strerror

#include "common_socket.h"
#include "app_thread.h"
#include "logger.h"
#include "platform_utils.h"
#include "app_config.h"

extern volatile bool shutdown_flag;

#define BUFFER_SIZE               8192
#define SOCKET_ERROR_BUFFER_SIZE  256
#define BLOCKING_TIMEOUT_SEC 10  // Blocking timeout in seconds


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
    while (!shutdown_flag) {
        logger_log(LOG_DEBUG, "Client Manager Attempting to connect to server %s on port %d...", hostname, port);
        SOCKET sock = setup_socket(is_server, is_tcp, addr, client_addr, hostname, port);
        if (sock == INVALID_SOCKET) {
            logger_log(LOG_ERROR, "Socket setup failed. Retrying in %d seconds...", backoff);
            {
                char error_buffer[SOCKET_ERROR_BUFFER_SIZE];
                get_socket_error_message(error_buffer, sizeof(error_buffer));
                logger_log(LOG_ERROR, "%s", error_buffer);
            }
            sleep(backoff);
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
                close_socket(&sock);
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
    if (shutdown_flag) {
		logger_log(LOG_INFO, "Client Manager attempt to connect exiting due to app shutdown.");
    }
    return INVALID_SOCKET;
}

/*
 * format_block() writes a formatted 32-bit block into dest.
 *
 * This function always produces 4 "byte slots" after the "0x" prefix. For each
 * carried slot, two dots ("..") are printed; for each new byte, its value is printed
 * in hex (using %02X). If there aren't enough bytes, the missing slots are filled with dots.
 */
static void format_block(char* dest, size_t dest_size, const uint8_t* new_bytes_ptr,
    int carried_bytes, int new_bytes) {
    int n = snprintf(dest, dest_size, "0x");
    // For carried (old) bytes, print dots
    for (int i = 0; i < carried_bytes; i++) {
        n += snprintf(dest + n, dest_size - n, "..");
    }
    // For the new bytes, print their hex values
    for (int i = 0; i < new_bytes; i++) {
        n += snprintf(dest + n, dest_size - n, "%02X", new_bytes_ptr[i]);
    }
    // Fill any remaining slots (to total 4 bytes) with dots
    int missing = 4 - (carried_bytes + new_bytes);
    for (int i = 0; i < missing; i++) {
        n += snprintf(dest + n, dest_size - n, "..");
    }
    // Append a space at the end
    snprintf(dest + n, dest_size - n, " ");
}

/*
 * log_buffered_data() logs the data accumulated since the last timeout.
 *
 */
static void log_buffered_data(char* buffer, int* buffered_length, int batch_bytes) {

    static int s_carry_length = 0;
    // Total new bytes to consider = carried (from previous log) + current buffered bytes.
    int total = s_carry_length + *buffered_length;
    if (total == 0 && batch_bytes == 0)
        return;

    logger_log(LOG_INFO, "%d bytes received: top", batch_bytes);

    int index = 0;
    char block_str[64];

    // If there are carried bytes from before, complete that block first.
    if (s_carry_length > 0) {
        int needed = 4 - s_carry_length;  // number of new bytes required to complete this block
        if (*buffered_length >= needed) {
            // Use the first 'needed' bytes from buffer to complete the block.
            format_block(block_str, sizeof(block_str), buffer, s_carry_length, needed);
            logger_log(LOG_INFO, "%s", block_str);
            index += needed;
            // Reset carried bytes since the block is now complete.
            s_carry_length = 0;
        }
        else {
            // Not enough new bytes to complete the block.
            // Log the incomplete block using all the new bytes.
            int new_bytes = *buffered_length;
            format_block(block_str, sizeof(block_str), buffer, s_carry_length, new_bytes);
            logger_log(LOG_INFO, "%s", block_str);
            // Update carried count (but do not retain actual bytes).
            s_carry_length += new_bytes;
            *buffered_length = 0;
            logger_log(LOG_INFO, "%d bytes received: bottom", batch_bytes);
            return;
        }
    }

    // Process complete blocks from the remaining new data.
    while (index + 4 <= *buffered_length) {
        format_block(block_str, sizeof(block_str), buffer + index, 0, 4);
        logger_log(LOG_INFO, "%s", block_str);
        index += 4;
    }

    // Process any leftover new bytes (incomplete block)
    if (index < *buffered_length) {
        int new_leftover = *buffered_length - index;
        format_block(block_str, sizeof(block_str), buffer + index, 0, new_leftover);
        logger_log(LOG_INFO, "%s", block_str);
        // Save the count (we do not retain the actual data)
        s_carry_length = new_leftover;
    }
    else {
        s_carry_length = 0;
    }

    // Clear the buffer.
    *buffered_length = 0;
    logger_log(LOG_INFO, "%d bytes received: bottom", batch_bytes);
}

/*
 * handle_data_reception() reads data from the socket, updating both the overall
 * buffered length and the count of new bytes (batch_bytes) since the last log.
 */
static bool handle_data_reception(SOCKET sock, char* buffer, int* buffered_length, int* batch_bytes) {
	// only read after a select, so we know data is available
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
void setup_select_timeout(SOCKET sock, fd_set* read_fds, struct timeval* timeout, long sec, long usec) {
    FD_ZERO(read_fds);
    FD_SET(sock, read_fds);
    timeout->tv_sec = sec;
    timeout->tv_usec = usec;
}

/**
 * @brief Monitors a socket for incoming data and processes it.
 *
 * This function uses a single loop. Initially, it uses a blocking select()
 * with a 5‑second timeout. When data arrives, it switches to non‑blocking
 * checks (timeout = 0) to drain all available data before reverting to blocking mode.
 *
 * @param sock        The socket descriptor to monitor.
 * @param is_server   Unused flag indicating if this is a server.
 * @param is_tcp      Unused flag indicating if the protocol is TCP.
 * @param client_addr Unused pointer to client address information.
 */
void client_comms_loop(SOCKET sock, int is_server, int is_tcp, struct sockaddr_in* client_addr) {
    bool valid = true;
    char buffer[BUFFER_SIZE];
    int buffered_length = 0;
    int batch_bytes = 0;

    // Flag to determine whether to block (10-second timeout) or perform a non-blocking check.
    bool blocking = true;

    while (!shutdown_flag) {
        fd_set read_fds;
        struct timeval timeout;

        // Use a blocking timeout if waiting for new data, otherwise non-blocking.
        if (blocking) {
            setup_select_timeout(sock, &read_fds, &timeout, BLOCKING_TIMEOUT_SEC, 0);
        }
        else {
            setup_select_timeout(sock, &read_fds, &timeout, 0, 0);
        }

        int ret = select((int)sock + 1, &read_fds, NULL, NULL, &timeout);

        if (ret < 0) {
            logger_log(LOG_ERROR, "Select error. Exiting loop.");
            char err_buf[SOCKET_ERROR_BUFFER_SIZE];
            get_socket_error_message(err_buf, sizeof(err_buf));
            logger_log(LOG_ERROR, "%s", err_buf);
            valid = false;
            close_socket(&sock);
            break;
        }

        if (ret == 0) {
            // If in non-blocking mode, a timeout means no more data is available.
            if (!blocking) {
                if (buffered_length > 0) {
                    log_buffered_data(buffer, &buffered_length, batch_bytes);
                }
                else {
                    logger_log(LOG_DEBUG, "Non-blocking check: No more data available");
                }
                // Reset to blocking mode for the next iteration.
                blocking = true;
                batch_bytes = 0;
            }
            else {
                // In blocking mode, a timeout simply means no data was received during the wait.
                logger_log(LOG_DEBUG, "Timeout: No data received within %d seconds", BLOCKING_TIMEOUT_SEC);
            }
            continue;
        }

        // If select() indicates that data is available on the socket.
        if (FD_ISSET(sock, &read_fds)) {
            if (!handle_data_reception(sock, buffer, &buffered_length, &batch_bytes)) {
                valid = false;
                close_socket(&sock);
                break;
            }
            // Data was received, so switch to non-blocking mode to drain any remaining data.
            blocking = false;
        }
    }

    if (shutdown_flag) {
        logger_log(LOG_DEBUG, "Shutdown requested, exiting communication loop.");
    }
}


/**
 * Main client thread entry point.
 */
void* clientMainThread(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    set_thread_label(thread_info->label);
    ClientThreadArgs_T* client_info = (ClientThreadArgs_T*)thread_info->data;

    // Check for the server hostname in the configuration.
    const char* config_server_hostname = get_config_string("network", "client.server_hostname", NULL);
    if (config_server_hostname) {
        strncpy(client_info->server_hostname, config_server_hostname, sizeof(client_info->server_hostname));
        client_info->server_hostname[sizeof(client_info->server_hostname) - 1] = '\0';
    }
    client_info->port = get_config_int("network", "client.port", client_info->port);
    logger_log(LOG_INFO, "Client Manager will attempt to connect to Server: %s, port: %d", client_info->server_hostname, client_info->port);

    int port = client_info->port;
    bool is_tcp = client_info->is_tcp;
    bool is_server = false;
    SOCKET sock = INVALID_SOCKET;

    struct sockaddr_in addr, client_addr;
    memset(&addr, 0, sizeof(addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // Attempt to connect to the server.
    sock = attempt_connection(is_server, is_tcp, &addr, &client_addr,
        client_info->server_hostname, port);
    if (sock == INVALID_SOCKET) {
        logger_log(LOG_INFO, "Shutdown requested before communication started.");
        return NULL;
    }

    // Start the communication loop.
    client_comms_loop(sock, is_server, is_tcp, &client_addr);
    close_socket(&sock);

    logger_log(LOG_INFO, "CLIENT: Exiting client thread.");
    return NULL;
}
