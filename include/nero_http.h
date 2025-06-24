#ifndef NERO_HTTP_H
#define NERO_HTTP_H

#include <stdbool.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 9000
#define USE_SSL 1

// --- Cross-platform socket abstraction ---
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define close_socket(s) closesocket(s)
#define poll_socket(fd, c, ms) WSAPoll(fd, c, ms)
typedef WSAPOLLFD socket_poll_fd;
typedef SOCKET socket_fd;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define close_socket(s) close(s)
#define poll_socket(fd, c, ms) poll(fd, c, ms)
typedef struct pollfd socket_poll_fd;
typedef int socket_fd;
#endif

// --- Debug / Logging Macros ---
#define HTTP_PRINT_ERROR(fd, msg, ...) \
    fprintf(fd, "Error: " msg " [%s:%d]\n", ##__VA_ARGS__, __FILE__, __LINE__)

#define HTTP_PRINT_SSL_ERROR(fd, msg, ...)        \
    {                                             \
        HTTP_PRINT_ERROR(fd, msg, ##__VA_ARGS__); \
        ERR_print_errors_fp(fd);                  \
    }

// --- HTTP Connection Structures ---
typedef struct HTTP_Connection
{
    socket_fd client;
    SSL *ssl;
    bool ended;
    pthread_t thread;
    bool *run;
    void **modules;
    struct HTTP_Connection *next;
    struct HTTP_Connection *prev;
} HTTP_Connection;

typedef struct
{
    HTTP_Connection *base;
    HTTP_Connection *head;
    size_t count;
    int limit;
    socket_fd server;
    SSL_CTX *ssl_ctx;
    bool run;
    void **modules;
    pthread_t thread;
} HTTP_Connection_Manager;

// --- Connection Management ---
HTTP_Connection *HTTP_Connection_Get(HTTP_Connection_Manager *context);
void HTTP_Connection_Destroy(HTTP_Connection **conn);
bool HTTP_Manager_Push_Connection(HTTP_Connection_Manager *context, HTTP_Connection *push);
bool HTTP_Manager_Remove_Connection(HTTP_Connection_Manager *context, HTTP_Connection *toRemove, int position, HTTP_Connection *startOver);
void HTTP_Manager_Connections(HTTP_Connection_Manager *context);
void HTTP_Manager_Destroy(HTTP_Connection_Manager *context);

// --- HTTP Header Structures ---
typedef struct HTTP_Header_Value
{
    char *name;
    char *value;
    struct HTTP_Header_Value *next;
} HTTP_Header_Value;

typedef struct
{
    char *prologue;
    HTTP_Header_Value *values;
    size_t count;
} HTTP_Header;

// --- Header Operations ---
HTTP_Header *HTTP_Header_GetFromClient(HTTP_Connection *conn);
HTTP_Header *HTTP_Header_CreateServerHeader();
const char *HTTP_Header_GetValue(HTTP_Header *header, const char *object);
bool HTTP_Header_Push(HTTP_Header *header, const char *name, const char *value, bool replace);
bool HTTP_Header_RemoveObject(HTTP_Header *header, const char *name, bool firstFind);
bool HTTP_Header_SendToClient(HTTP_Connection *conn, HTTP_Header *header, int status_code);
void HTTP_Header_Destroy(HTTP_Header **header);
void HTTP_Header_Print(HTTP_Header *header);

// --- HTTP IO ---
int HTTP_Write(HTTP_Connection *conn, const char *data, size_t length);
int HTTP_Read(HTTP_Connection *conn, char *buffer, size_t length);
void *HTTP_HandleConnection(HTTP_Connection *conn);

// --- URL / Path Mapping ---
typedef struct
{
    const char *method;
    const char *verb;
    const char **path;
    size_t count;
    void *internal;
} HTTP_Map;

HTTP_Map *HTTP_Map_Get(HTTP_Header *header);
void HTTP_Map_Print(HTTP_Map *map);
void HTTP_Map_Destroy(HTTP_Map **map);

#endif // NERO_HTTP_H
