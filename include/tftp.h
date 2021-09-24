#ifndef TFTP_H
#define TFTP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "../include/buffer.h"

typedef struct
{
	int socket;
	struct addrinfo *ConnectNode;
	struct sockaddr_storage server_addr;
	socklen_t server_addr_size;
	buffer_t *readbuf; 
	buffer_t *writebuf;
} tftp_t;

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
} TFTP_TRANSFER_TYPES;

int tftp_init(tftp_t *node);

int tftp_loop(int argc, char *argv[]);

int tftp_connect(tftp_t *tftp_node, char *ipaddress, char* port);

int tftp_close(tftp_t *node);

int tftp_send_read_request(tftp_t *node, const char *filename, const char *mode);

int tftp_send_write_request(tftp_t *node, const char *filename, const char *mode);

int tftp_send_data(tftp_t *node, int fd, int block);

int tftp_send_ack(tftp_t *node, uint16_t block);

int tftp_send_file(tftp_t *node, char *filename);

int tftp_recv_file(tftp_t *node, char *filename);

#endif
