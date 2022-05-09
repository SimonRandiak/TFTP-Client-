#include "../include/tftp.h"
#include "../include/tools.h"
#if defined _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined __linux__
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const static char *tftp_commands[] = 
{
	"PUT",
	"GET",
	"QUIT",
	"OPEN",
	"BINARY",
	"NETASCII",
	"HELP",
};

const static char *tftp_codes[] = 
{
   "Not defined, see error message (if any).",
   "File not found.",
   "Access violation.",
   "Disk full or allocation exceeded.",
   "Illegal TFTP operation.",
   "Unknown transfer ID.",
   "File already exists.",
   "No such user.",	
};

int tftp_loop(int argc, char *argv[])
{

	tftp_t node;
	tftp_init(&node);

	if (argc > 2)
	{
		strncpy(node.ip, argv[1], 261);
		strncpy(node.port, argv[2], 6);
		tftp_connect(&node, argv[1], argv[2]);
	}
	else if (argc == 2)
	{
		strncpy(node.ip, argv[1], 261);
		tftp_connect(&node, argv[1], "69");
	}
	buffer_t *buf = buffer_allocate(1024);	
	while (1)
	{
		printf("tftp> ");
		get_user_input(buf);
		
		if(buf->current_size > 0)
		{
			if ( strcasecmp(strtok(buf->data, " "), tftp_commands[0]) == 0)
			{
				if (node.connection_state == TFTP_DISCONNECTED)
					continue;
				else
					tftp_connect(&node, node.ip, node.port);
				char *filename = buf->data+4;
				tftp_send_file(&node, filename);	
			}
			else if  ( strcasecmp(strtok(buf->data, " "), tftp_commands[1]) == 0)
			{
				if (node.connection_state == TFTP_DISCONNECTED)
					continue;
				else
					tftp_connect(&node, node.ip, node.port);
				char *filename = buf->data+4;
				
				tftp_recv_file(&node, filename);
			}
			else if ( strcasecmp(strtok(buf->data, " "), tftp_commands[2]) == 0)
			{
				fprintf(stderr, "tftp: closing\n"); 
				break;	
			}
			else if ( strcasecmp(strtok(buf->data, " "), tftp_commands[3]) == 0)
			{
				char *host = strtok(buf->data + 5, " ");
				if (host == NULL)
				{
					fprintf(stderr, "open hostname <port>\n");
					continue;
				}
				else
				{
					strncpy(node.ip, host, 261);
				}
				char *port = strtok(NULL, " ");
				if (port == NULL)
					strncpy(node.port, "69", 6);
				else
					strncpy(node.port, port, 6);
				tftp_connect(&node, node.ip, node.port);
			}
			else if ( strcasecmp(strtok(buf->data, " "), tftp_commands[4]) == 0)
			{
				node.transfer_mode = TFTP_BINARY_MODE;
			}
			else if ( strcasecmp(strtok(buf->data, " "), tftp_commands[5]) == 0)
			{
				node.transfer_mode = TFTP_NETASCII_MODE;
			}
			else if ( strcasecmp(strtok(buf->data, " "), tftp_commands[6]) == 0)
			{
				fprintf(stdout, "\t\t\t\tput\t\t<file> send file to the server\n \
				get\t\t<file> get file from the server\n \
				quit\t\t<none> quit program\n \
				open\t\t<host> port (optional) connect to server\n \
				binary\t\t<none> set transfer mode to binary\n \
				netascii\t<none> set transfer mode to netascii\n \
				help\t\t<none> show help\n");
			
			}
			else
			{
				fprintf(stderr, "tftp: Invalid Command: %s\n", strtok(buf->data, " "));	
			}

		}
	}
	buffer_release(buf);
	tftp_close(&node);
	return 0;
	
}

int tftp_init(tftp_t *node)
{
#if defined _WIN32
	WSADATA wsaData;

	// Initialize Winsock
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
	{
	    puts("WSAStartup failed");
	    return 1;
	}
#endif

	memset(node, 0, sizeof (tftp_t));
	node->writebuf = buffer_allocate(2048);
	node->readbuf = buffer_allocate(2048);
	node->server_addr_size = sizeof (struct sockaddr_storage);
	node->socket = -1;
	memset(node->ip, 0, 262);
	memset(node->port, 0, 7);
	node->connection_state = TFTP_DISCONNECTED;
	node->transfer_mode = TFTP_BINARY_MODE;
}

int tftp_connect(tftp_t *tftp_node, char *ipaddress, char* port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
#if defined _WIN32
	DWORD timeout = 80000;
#elif defined __linux__
	struct timeval timeout;
	timeout.tv_sec = 80;
	timeout.tv_usec = 0;
#endif
    int sfd, s;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    
    hints.ai_socktype = SOCK_DGRAM; 
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;          

	s = getaddrinfo(ipaddress, port, &hints, &result);
    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
   	    if (sfd == -1)
	    {
#if defined _WIN32
			closesocket(sfd);
#elif defined __linux__
		    close(sfd);
#endif
		continue;
		}
		tftp_node->socket = sfd;
		tftp_node->ConnectNode = rp;
		tftp_node->connection_state = TFTP_CONNECTED;
		break;
	}

	if (rp == NULL)
    {               
		fprintf(stderr, "Could not connect\n");
		return -1;
    }
#if defined _WIN32
	if (setsockopt (sfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof timeout) == SOCKET_ERROR)
	{
        fprintf(stderr, "setsockopt: %d\n", WSAGetLastError());
		return -1;
	}
#elif defined __linux__
  	if (setsockopt (sfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0)
	{
        fprintf(stderr, "setsockopt: %s\n", strerror(errno));
		return -1;
	}
#endif
}

int tftp_close(tftp_t *node)
{
#if defined _WIN32
	closesocket(node->socket);
#elif defined __linux__
	close(node->socket);
#endif
	node->socket = -1;

    buffer_release(node->writebuf);
	buffer_release(node->readbuf);

	freeaddrinfo(node->ConnectNode);
	node->connection_state = TFTP_DISCONNECTED;
	return 1;
}

static int tftp_send_read_request(tftp_t *node, const char *filename)
{
	const char *mode = NULL;
	switch (node->transfer_mode)
	{
		case TFTP_BINARY_MODE:
			mode = "octet";
			break;
		case TFTP_NETASCII_MODE:
			mode = "netascii";
			break;
		default:
			fprintf(stderr, "tftp: unsuported transfer mode\n");
			return -1;
	}
	size_t packet_len = strlen(filename) + strlen(mode) + 4;

	memset(node->writebuf->data,0, packet_len);

	node->writebuf->data[1] = TFTP_RRQ;

	memcpy(node->writebuf->data + 2, filename, strlen(filename));

	memcpy(node->writebuf->data + 3 + strlen(filename), mode, strlen(mode));

	if (!udp_send_all_data(node->socket, node->writebuf->data, packet_len, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen))
			return -1;
	return 1;
}

static int tftp_send_write_request(tftp_t *node, const char *filename)
{
	const char *mode = NULL;
	switch (node->transfer_mode)
	{
		case TFTP_BINARY_MODE:
			mode = "octet";
			break;
		case TFTP_NETASCII_MODE:
			mode = "netascii";
			break;
		default:
			fprintf(stderr, "tftp: unsuported transfer mode\n");
			return -1;
	}

	size_t packet_len = strlen(filename) + strlen(mode) + 4;

	memset(node->writebuf->data,0, packet_len);

	node->writebuf->data[1] = TFTP_WRQ;

	memcpy(node->writebuf->data + 2, filename, strlen(filename));

	memcpy(node->writebuf->data + 3 + strlen(filename), mode, strlen(mode));

	if(!udp_send_all_data(node->socket, node->writebuf->data, packet_len, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen))
		return -1;

	if(!udp_recv_data(node->socket, node->readbuf->data, node->readbuf->max_size, (struct sockaddr *) &node->server_addr, &node->server_addr_size))
		return -1;

	if ((uint8_t) node->readbuf->data[1] == TFTP_ACK)
	{
		return 1;
	}
	return -1;


}
static int tftp_send_data(tftp_t *node, FILE *fd, int block)
{
	int nread = -1;

	buffer_clear(node->writebuf);

	node->writebuf->data[1] = TFTP_DATA;
	block = htons(block);

	memcpy(&node->writebuf->data[2], &block, sizeof(uint16_t));

	nread = fread(&node->writebuf->data[4], 1, 512, fd);
	
	if (nread < 0)
	{
		perror("fread:");
		return -1;
	}

	if (!udp_send_all_data(node->socket, node->writebuf->data, nread + 4, (struct sockaddr *) &node->server_addr, node->server_addr_size))
	{
		return -1;
	}	
	return nread;
}

static int tftp_send_ack(tftp_t *node, uint16_t block)
{
	buffer_clear(node->writebuf);
	node->writebuf->data[1] = TFTP_ACK;

	block = htons(block);

	memcpy(&node->writebuf->data[2], &block, sizeof(uint16_t));

	node->writebuf->current_size = 4;

	if (!udp_send_all_data(node->socket, node->writebuf->data, 4, (struct sockaddr *) &node->server_addr, node->server_addr_size))
		return -1;
}

int tftp_send_file(tftp_t *node, char *filename)
{
	FILE *fd = NULL;
	
	int nwrite = 0;
	unsigned int totalread = 0;
	uint16_t block = 1;

	fd = fopen(filename, "rb");

	if (fd == NULL)
	{
		perror("fopen:");
		return -1;
	}

	fseek(fd, 0, SEEK_END);
	int file_sz = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	if (!tftp_send_write_request(node, filename))
	{
		return -1;
	}

	while(1)
	{

		nwrite = tftp_send_data(node, fd, block);
		if (nwrite == -1)
			return -1;
		totalread += nwrite;
		
		nwrite = udp_recv_data(node->socket, node->writebuf->data, node->writebuf->max_size, (struct sockaddr *) &node->server_addr, &node->server_addr_size);	
		if (nwrite == -1)
		{
			return -1;
		}
		if (node->readbuf->data[1] == TFTP_ACK)
		{

			uint16_t recvblock = *(uint16_t *)&node->writebuf->data[2];
			if (ntohs(recvblock) == block)
			{
				block++;
			}
			else
			{
				udp_send_data(node->socket, node->writebuf->data, node->writebuf->current_size, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen);
				
			}
			
		}
			if (totalread >= file_sz)
				break;
	}
	printf("sent: %d bytes\n", totalread);
	fclose(fd);
}

int tftp_recv_file(tftp_t *node, char *filename)
{
	FILE *fd = NULL;

	uint16_t block = 1;
	int nread = 0;
	int totalread = 0;


	fd = fopen(filename, "wb");
	if (fd == NULL)
	{
		perror("fopen:");
		return -1;
	}

	if (!tftp_send_read_request(node, filename))
		return -1;

	while(1)
	{
		nread = udp_recv_data(node->socket, node->readbuf->data, 516, (struct sockaddr *) &node->server_addr, &node->server_addr_size); 

		if (node->readbuf->data[1] == TFTP_DATA)
		{
			uint16_t recvblock = *(uint16_t *)&node->readbuf->data[2];
			if (ntohs(recvblock) == block)
			{
				tftp_send_ack(node, block);
				block++;
			}
			else
			{
				tftp_send_ack(node, block);
				continue;
			}
			
		}

		fwrite(&node->readbuf->data[4], 1, nread - 4, fd);
		totalread += nread;

		if ((nread - 4) < 512)
			break;

	}
	fclose(fd);

	printf("%d bytes has been written to file: %s\n", totalread, filename);
}
