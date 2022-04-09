#ifndef TOOLS_H
#define TOOLS_H
#if defined _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined __linux__
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <stdlib.h>

int udp_recv_data(int socket, void *data, const size_t data_len, struct sockaddr *to_addr, socklen_t *to_addr_size);

int udp_send_data(int socket, const void *data, const size_t data_len, struct sockaddr *to_addr, socklen_t to_addr_size);

int udp_recv_all_data(int socket, void *data, const size_t data_len, struct sockaddr *to_addr, socklen_t *to_addr_size);

int udp_send_all_data(int socket, const void *data, const size_t data_len, struct sockaddr *to_addr, socklen_t to_addr_size);

#endif
