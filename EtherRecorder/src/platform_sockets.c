/**
 * @file platform_sockets.c
 * @brief Platform-specific socket initialization and cleanup functions.
 */

#include "platform_sockets.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#else
// #include <fcntl.h> // For fcntl on Unix
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the socket library.
 *
 * On Windows, this function initializes the Winsock library. On other platforms,
 * it does nothing.
 */
void initialise_sockets(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Winsock initialisation failed. Error: %d\n", WSAGetLastError());
        exit(1);
    }
#endif
}

/**
 * @brief Cleans up the socket library.
 *
 * On Windows, this function cleans up the Winsock library. On other platforms,
 * it does nothing.
 */
void cleanup_sockets(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

int platform_getsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen) {
#ifdef _WIN32
    // On Windows, optlen is of type int
    int optlen_windows = (int)*optlen;
    int result = getsockopt(sock, level, optname, (char *)optval, &optlen_windows);
    if (result == 0) {
        *optlen = (socklen_t)optlen_windows; // Update optlen to match POSIX type
    }
    return result;
#else
    // On Linux, directly use getsockopt
    return getsockopt(sock, level, optname, optval, optlen);
#endif
}


/**
 * @brief Prints a socket error message.
 *
 * On Windows, this function prints the provided message followed by the last
 * Winsock error code. On other platforms, it uses `perror` to print the message
 * and the last error.
 *
 * @param msg The error message to print.
 */
void print_socket_error(const char *msg) {
#ifdef _WIN32
    fprintf(stderr, "%s: Error %d\n", msg, WSAGetLastError());
#else
    perror(msg);
#endif
}


#include "platform_sockets.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #define CLOSESOCKET closesocket
#else
    #define CLOSESOCKET close
#endif

/**
 * @brief Sets a socket to non-blocking mode.
 *
 * On Windows, this function uses `ioctlsocket` to set the socket to non-blocking mode.
 * On other platforms, it uses `fcntl` to set the `O_NONBLOCK` flag.
 *
 * @param sock The socket to set to non-blocking mode.
 * @return 0 on success, or -1 on error.
 */
int set_non_blocking_mode(SOCKET sock) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

/**
 * @brief Restores a socket to blocking mode.
 *
 * On Windows, this function uses `ioctlsocket` to set the socket to blocking mode.
 * On other platforms, it uses `fcntl` to clear the `O_NONBLOCK` flag.
 *
 * @param sock The socket to restore to blocking mode.
 * @return 0 on success, or -1 on error.
 */
int restore_blocking_mode(SOCKET sock) {
#ifdef _WIN32
    u_long mode = 0;
    return ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
#endif
}

/**
 * Returns a human-readable string for platform socket errors.
 */
const char* platform_socket_strerror(PlatformSocketError error) {
    switch (error) {
        case PLATFORM_SOCKET_SUCCESS:
            return "Success";
        case PLATFORM_SOCKET_ERROR_CREATE:
            return "Error creating socket";
        case PLATFORM_SOCKET_ERROR_RESOLVE:
            return "Error resolving address";
        case PLATFORM_SOCKET_ERROR_BIND:
            return "Error binding socket";
        case PLATFORM_SOCKET_ERROR_LISTEN:
            return "Error listening on socket";
        case PLATFORM_SOCKET_ERROR_CONNECT:
            return "Error connecting to socket";
        case PLATFORM_SOCKET_ERROR_TIMEOUT:
            return "Socket operation timed out";
        case PLATFORM_SOCKET_ERROR_SEND:
            return "Error sending data";
        case PLATFORM_SOCKET_ERROR_RECV:
            return "Error receiving data";
        case PLATFORM_SOCKET_ERROR_SELECT:
            return "Error with select operation";
        case PLATFORM_SOCKET_ERROR_GETSOCKOPT:
            return "Error getting socket options";
        default:
            return "Unknown socket error";
    }
}

#ifdef __cplusplus
}
#endif
