#ifndef TFTP_H
#define TFTP_H

#if defined _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <stdint.h>
#include <stdio.h>

#include "../include/buffer.h"

typedef enum 
{
	TFTP_RRQ = 1,
	TFTP_WRQ = 2,
	TFTP_DATA = 3,
	TFTP_ACK = 4,
	TFTP_ERROR = 5,
} TFTP_MODES;

typedef enum
{
	TFTP_NETASCII_MODE = 1,
	TFTP_BINARY_MODE = 0,
} TFTP_TRANSFER_MODES;

typedef enum
{
	TFTP_DISCONNECTED = 0,
	TFTP_CONNECTED = 1,
} TFTP_CONNECT_STATE;

typedef struct
{
	TFTP_CONNECT_STATE connection_state;
	int socket;
	struct addrinfo *ConnectNode;
	struct sockaddr_storage server_addr;
	socklen_t server_addr_size;
	buffer_t *readbuf; 
	buffer_t *writebuf;
	char ip[262];
	char port[7];
	TFTP_TRANSFER_MODES transfer_mode;
} tftp_t;


int tftp_init(tftp_t *node);

int tftp_loop(int argc, char *argv[]);

int tftp_connect(tftp_t *tftp_node, char *ipaddress, char* port);

int tftp_close(tftp_t *node);

static int tftp_send_read_request(tftp_t *node, const char *filename);

static int tftp_send_write_request(tftp_t *node, const char *filename);

static int tftp_send_data(tftp_t *node, FILE *fd, int block);

static int tftp_send_ack(tftp_t *node, uint16_t block);

int tftp_send_file(tftp_t *node, char *filename);

int tftp_recv_file(tftp_t *node, char *filename);

#endif
