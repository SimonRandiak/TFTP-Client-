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

int tftp_init(tftp_t *node);

int tftp_loop(int argc, char *argv[]);

int tftp_connect(tftp_t *tftp_node, char *ipaddress, char* port);

int tftp_close(tftp_t *node);

int tftp_send_ack(tftp_t *node, uint16_t block);

void tftp_make_file_req(tftp_t *node, TFTP_MODES opcode, char *filename, char *mode);

int tftp_send_file(tftp_t *node, char *filename);

int tftp_recv_file(tftp_t *node, char *filename);

#endif
