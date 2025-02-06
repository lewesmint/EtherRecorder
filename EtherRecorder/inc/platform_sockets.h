/**
 * @file platform_sockets.h
 * @brief Platform-specific socket initialization and cleanup functions.
 */

#ifndef PLATFORM_SOCKETS_H
#define PLATFORM_SOCKETS_H

// Platform-independent socket definitions
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define CLOSESOCKET closesocket
    typedef int socklen_t;
#else

    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <netdb.h> // For getaddrinfo and freeaddrinfo
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define CLOSESOCKET close
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PlatformSocketError {
    PLATFORM_SOCKET_SUCCESS = 0,
    PLATFORM_SOCKET_ERROR_CREATE = -1,
    PLATFORM_SOCKET_ERROR_RESOLVE = -2,
    PLATFORM_SOCKET_ERROR_BIND = -3,
    PLATFORM_SOCKET_ERROR_LISTEN = -4,
    PLATFORM_SOCKET_ERROR_CONNECT = -5,
    PLATFORM_SOCKET_ERROR_TIMEOUT = -6,
    PLATFORM_SOCKET_ERROR_SEND = -7,
    PLATFORM_SOCKET_ERROR_RECV = -8,
    PLATFORM_SOCKET_ERROR_SELECT = -9,
    PLATFORM_SOCKET_ERROR_GETSOCKOPT = -10
} PlatformSocketError;


#ifdef _WIN32
    #define PLATFORM_SOCKET_WOULDBLOCK WSAEWOULDBLOCK
    #define PLATFORM_SOCKET_INPROGRESS WSAEINPROGRESS
#else
    #define PLATFORM_SOCKET_WOULDBLOCK EWOULDBLOCK
    #define PLATFORM_SOCKET_INPROGRESS EINPROGRESS
#endif

// Macros for socket error retrieval
#ifdef _WIN32
    #define GET_LAST_SOCKET_ERROR() WSAGetLastError()
#else
    #define GET_LAST_SOCKET_ERROR() errno
#endif

// Socket utility functions

/**
 * @brief Initializes the socket library.
 *
 * On Windows, this function initializes the Winsock library. On other platforms,
 * it does nothing.
 */
void initialise_sockets(void);

/**
 * @brief Cleans up the socket library.
 *
 * On Windows, this function cleans up the Winsock library. On other platforms,
 * it does nothing.
 */
void cleanup_sockets(void);

int platform_getsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen);

/**
 * @brief Prints a socket error message.
 *
 * On Windows, this function prints the provided message followed by the last
 * Winsock error code. On other platforms, it uses `perror` to print the message
 * and the last error.
 *
 * @param msg The error message to print.
 */
void print_socket_error(const char *msg);

/**
 * @brief Sets a socket to non-blocking mode.
 *
 * On Windows, this function uses `ioctlsocket` to set the socket to non-blocking mode.
 * On other platforms, it uses `fcntl` to set the `O_NONBLOCK` flag.
 *
 * @param sock The socket to set to non-blocking mode.
 * @return 0 on success, or -1 on error.
 */
int set_non_blocking_mode(SOCKET sock);

/**
 * @brief Restores a socket to blocking mode.
 *
 * On Windows, this function uses `ioctlsocket` to set the socket to blocking mode.
 * On other platforms, it uses `fcntl` to clear the `O_NONBLOCK` flag.
 *
 * @param sock The socket to restore to blocking mode.
 * @return 0 on success, or -1 on error.
 */
int restore_blocking_mode(SOCKET sock);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_SOCKETS_H
