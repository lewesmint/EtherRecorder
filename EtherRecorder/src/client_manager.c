#include "client_manager.h"

#include <stdio.h> // for perror

#include "common_socket.h"
#include "app_thread.h"
#include "logger.h"
#include "platform_utils.h"
#include "app_config.h"


extern volatile bool shutdown_flag;

#define BIG_SIZE 1024

#define BUFFER_SIZE 256

void printW_Wsa_error() {
    int errorCode = WSAGetLastError();

    fprintf(stderr, "Sockects failed with error: %d\n", errorCode);

    char errorMsg = NULL;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&errorMsg,
        0,
        NULL
    );
    fprintf(stderr, "Error message: %s\n", errorMsg);
    LocalFree(errorMsg);

}

int main(void) {
    // Example buffer containing at least 12 bytes
    uint8_t buffer[12] {
        0xA1, 0xA2, 
        0xA3, 0xA4, 
        0xA5, 0xA6, 
        0xA7, 0xA8, 
        0xA9, 0xAA, 
        0xAB, 0xAC
    }

    // Prin the first 6 pairs of bytes as 16-bit integers in hex
    for (int i = 0; i < 6; i++) {
        uint16_t value = (buffer[i * 2] << 8) | buffer[i * 2 + 1];
        printf("0x%04X\n", value);
    }
}

///////
///////

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")  // Link against Winsock library

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connect failed: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    // ✅ Initialize `select()` structures
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 5;   // 5-second timeout
    timeout.tv_usec = 0;  // 0 microseconds

    printf("Waiting for data...\n");

    int ret = select(0, &read_fds, NULL, NULL, &timeout);
    if (ret == 0) {
        printf("Timeout: No data received within 5 seconds\n");
    } else if (ret > 0) {
        // Data is available, call recv()
        char buffer[1024];
        int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Received: %s\n", buffer);
        } else if (bytes_received == 0) {
            printf("Connection closed by peer\n");
        } else {
            printf("recv failed: %d\n", WSAGetLastError());
        }
    } else {
        printf("select failed: %d\n", WSAGetLastError());
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}

/////// 
///////

int do process_tcp_atream(SOCKET soxk, uint32_t *out_buffer, int out_buffer_size) {


    int ret = select(sock + 1, &read_fds, NULL, NULL, &timeout);
    if (ret == 0) {
        logger_log(LOG_DEBUG, "Timeout: No data received within 5 seconds");
        return 0;
    } else if (ret > 0) {
        // Data is available, call recv()
        int bytes_received = recv(sock, buffer + buffered_length, bytes_to_read, 0);
        if (bytes_received > 0) {
            logger_log(LOG_DEBUG, "Received: %s", buffer);
        } else if (bytes_received == 0) {
            logger_log(LOG_DEBUG, "Connection closed by peer");
        } else {
            logger_log(LOG_ERROR, "recv failed: %d", WSAGetLastError());
        }
    } else {
        logger_log(LOG_ERROR, "select failed: %d", WSAGetLastError());
    }
    while (1) {
        int bytes_to_read = BUFFER_SIZE - buffered_length;
        if (bytes_to_read <= 0) {
            logger_log(LOG_ERROR, "Buffer overflow; clearing buffer.");
            buffered_length = 0;
            continue;
        }
        int bytes_received = recv(sock, buffer + buffered_length, bytes_to_read, 0);
        if (bytes_received <= 0) {
            logger_log(LOG_ERROR, "recv error or connection closed (received %d bytes)", bytes_received);
            return -1;
        }
        logger_log(LOG_DEBUG, "Received %d bytes", bytes_received);
        buffered_length += bytes_received;

        while (1) {
            int start_index = find_marker_in_buffer(buffer, buffered_length, START_MARKER);
            if (start_index < 0) {
                logger_log(LOG_DEBUG, "No START_MARKER in current batch");
                if (buffered_length > 3) {
                    memmove(buffer, buffer + buffered_length - 3, 3);
                    buffered_length = 3;
                }
                break;
            }
            logger_log(LOG_DEBUG, "Found START_MARKER index = %d");
            if (start_index > 0) {
                memmove(buffer, buffer + start_index, buffered_length - start_index);
                buffered_length -= start_index;
            }

            if (buffered_length < 8) {
                break;
            }

            unsigned int payload_length = 0;
            memcpy(&payload_length, buffer + 4, sizeof(payload_length));

            unsigned int total_packet_size = 4 + 4 + payload_length + 4;
            if (buffered_length < (int)total_packet_size) {
                break;
            }

            // Process the packet
            process_packet(buffer + 4, payload_length);

            // Discard the processed packet
            memmove(buffer, buffer + total_packet_size, buffered_length - total_packet_size);
            buffered_length -= total_packet_size;
        }
    }
}

/**
 * The main communication loop, ensuring full sends/receives.
 */
void communication_loop(SOCKET sock, int is_server, int is_tcp, struct sockaddr_in *client_addr) {
    int addr_len = sizeof(struct sockaddr_in);
    bool valid_socket_connection = true;

    while (valid_socket_connection) {
        // Check for shutdown condition if needed.
        if (shutdown_flag) {
            logger_log(LOG_DEBUG, "Shutdown requested, exiting communication loop.");
            break;
        }

        DummyPayload sendPayload;
        int send_buffer_len = generateRandomData(&sendPayload);
        if (send_buffer_len > 0) {
            logger_log(LOG_DEBUG, "Sending %d bytes of data.", send_buffer_len);
            if (send_all_data(sock, &sendPayload, send_buffer_len, is_tcp, client_addr, addr_len) != 0) {
                logger_log(LOG_ERROR, "Send error. Exiting loop.");
                valid_socket_connection = false;
                close_socket(&sock);
                break;
            }
            logger_log(LOG_DEBUG, "Sleeping in comms loop");
            sleep_ms(100);
            logger_log(LOG_DEBUG, "Woken in comms loop");
        }

        DummyPayload receivePayload;
        int receive_buffer_len = sizeof(DummyPayload);
        logger_log(LOG_DEBUG, "Entering receive_all_data");
        int packet_size = receive_all_data(sock, &receivePayload, receive_buffer_len, is_tcp, client_addr, &addr_len);
        if (packet_size <= 0) {
            logger_log(LOG_ERROR, "Receive error. Exiting loop.");
            valid_socket_connection = false;
            close_socket(&sock);
            break;
        }
        logger_log(LOG_DEBUG, "Received complete packet (%d bytes)", packet_size);

        // Extract payload length from the received packet.
        unsigned int payload_length = 0;
        memcpy(&payload_length, ((char *)&receivePayload) + 4, sizeof(payload_length));

        // Process the received dummy packet.
        // The payload is located after the start marker and the length field (i.e. at offset 8).
        process_payload(((char *)&receivePayload) + 8, payload_length);

        logger_log(LOG_DEBUG, "Bottom of comms loop while valid connection");
    }
}



void* clientMainThread(void* arg) {
    AppThreadArgs_T* thread_info = (AppThreadArgs_T*)arg;
    set_thread_label(thread_info->label);
    ClientThreadArgs_T* client_info = (ClientThreadArgs_T*)thread_info->data;

    // see if the server_hostname is in the config
    const char* config_server_hostname = get_config_string("network", "client.server_hostname", NULL);

    // TODO Verify that this palavar is neccesary, having client_indo_server
    if (config_server_hostname != config_server_hostname) {
        strncpy(client_info->client_hostname, config_server_hostname, sizeof(client_info->client_hostname));
        client_info->client_hostname[sizeof(client_info->client_hostname) - 1] = '\0';
    }

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
        logger_log(LOG_DEBUG, "CLIEN MAnagerAttempting to connect to server %s on port %d...", client_info->client_hostname, port);
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
    client_comms_loop(sock, is_server, is_tcp, &client_addr);
    close_socket(&sock);

    logger_log(LOG_INFO, "CLIENT: Exiting client thread.");
    return NULL;
}

