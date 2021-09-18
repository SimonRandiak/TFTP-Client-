#include "../include/tftp.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

const static char *tftp_commands[] = 
{
	"PUT",
	"GET",
	"QUIT",
	"OPEN",
};

const static char *tftp_transfer_modes[] = 
{
	"octet",
	"mail",
	"netascii",
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
	if (argc > 1)
		tftp_connect(&node, argv[1], argv[2]);

	buffer_t *buf = buffer_allocate(1024);	
	while (1)
	{
		printf("tftp> ");
		get_user_input(buf);
		
		if(buf->current_size > 0)
		{
			if ( strcasecmp(strtok(buf->data, " "), tftp_commands[0]) == 0)
			{
				tftp_make_file_req(&node, TFTP_WRQ, buf->data+4, "octet");
				tftp_send_file(&node, buf->data+4);
			}
			else if  ( strcasecmp(strtok(buf->data, " "), tftp_commands[1]) == 0)
			{
				tftp_make_file_req(&node, TFTP_RRQ, buf->data+4, "octet");
				tftp_recv_file(&node, buf->data+4);
			}
			else if ( strcasecmp(strtok(buf->data, " "), tftp_commands[2]) == 0)
			{
				fprintf(stderr, "Closing tftp client\n"); 
				break;	
			}
			else if ( strcasecmp(strtok(buf->data, " "), tftp_commands[3]) == 0)
			{
				char ip[256] = {0};
				char cport[6] = {0};

				sscanf(buf->data+5, "%253s %5s", ip, cport);
				tftp_connect(&node, ip, cport);
			}
			else
			{
				fprintf(stdout, "Invalid Command: %s\n", strtok(buf->data, " "));	
			}

		}
	}
	buffer_release(buf);
	tftp_close(&node);
	return 0;
	
}

int tftp_init(tftp_t *node)
{
	memset(node, 0, sizeof (tftp_t));

	node->writebuf = buffer_allocate(2048);
	node->readbuf = buffer_allocate(2048);

	node->socket = -1;
	
}

int tftp_connect(tftp_t *tftp_node, char *ipaddress, char* port)
{
       	struct addrinfo hints;
    	struct addrinfo *result, *rp;
    	int sfd, s;
   
    	memset(&hints, 0, sizeof(struct addrinfo));
    	hints.ai_family = AF_UNSPEC;    
    	hints.ai_socktype = SOCK_DGRAM; 
    	hints.ai_flags = 0;
    	hints.ai_protocol = 0;          

    	if (port == NULL || port == " ")
		port = "69";
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
			close(sfd);
			continue;
		}
		tftp_node->socket = sfd;
		tftp_node->ConnectNode = rp;
		break;
	
    	}

    	if (rp == NULL)
    	{               
        	fprintf(stderr, "Could not connect\n");
        	exit(EXIT_FAILURE);
    	}
	fprintf(stdout, "Connecting to %s:%s\n", ipaddress, port);
}

int tftp_close(tftp_t *node)
{
	close(node->socket);
	node->socket = -1;

        buffer_release(node->writebuf);
	buffer_release(node->readbuf);

	freeaddrinfo(node->ConnectNode);
	return 1;
}

void tftp_make_file_req(tftp_t *node, TFTP_MODES opcode, char *filename, char *mode)
{
	int len = strlen(filename) + strlen(mode) + 5;
	if (len > node->writebuf->max_size)
		return;
	else 
		buffer_clear(node->writebuf);
	switch (opcode)
	{
		case TFTP_RRQ:
			node->writebuf->data[1] = TFTP_RRQ;

		        strncpy(node->writebuf->data+2, filename, strlen(filename));
			strncpy(node->writebuf->data + strlen(filename)+ 3, mode, strlen(mode));

			sendto(node->socket, node->writebuf->data, len-1, 0, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen);
			break;
		case TFTP_WRQ:
			socklen_t their_addr_size = sizeof(node->server_addr);
			node->writebuf->data[1] = TFTP_WRQ;

		        strncpy(node->writebuf->data+2, filename, strlen(filename));
			strncpy(node->writebuf->data + strlen(filename)+ 3, mode, strlen(mode));

			sendto(node->socket, node->writebuf->data, len-1, 0, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen);

			recvfrom(node->socket, node->readbuf->data, 4, 0, (struct sockaddr *) &node->server_addr, &their_addr_size);

			if (node->readbuf->data[1] == TFTP_ACK)
			{

				uint16_t recvblock = *(uint16_t *)&node->readbuf->data[2];
				if (recvblock != 0)
				{
					return;
				}
			
			}
	
			break;
	}
}

int tftp_send_data(tftp_t *node, int fd, int block)
{
	int nread = -1;

	buffer_clear(node->writebuf);

	node->writebuf->data[1] = TFTP_DATA;
	block = htons(block);

	memcpy(&node->writebuf->data[2], &block, sizeof(uint16_t));
	nread = read(fd, &node->writebuf->data[4], 512);

	node->writebuf->current_size = nread + 4;
	if (nread < 0)
	{
		fprintf(stderr, "sendto: %s\n", strerror(errno));
		return -1;
	}

	node->writebuf->current_size = nread + 4;

	nread = sendto(node->socket, node->writebuf->data, node->writebuf->current_size, 0, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen);
	if (nread < 0)
	{
		fprintf(stderr, "sendto: %s\n", strerror(errno));
		return -1;
	}
	else if (nread == 0)
	{

		fprintf(stdout, "sendto: shut down\n");
		return -1;
	}
	return nread;
}

int tftp_send_ack(tftp_t *node, uint16_t block)
{

	buffer_clear(node->writebuf);
	node->writebuf->data[1] = TFTP_ACK;

	block = htons(block);

	memcpy(&node->writebuf->data[2], &block, sizeof(uint16_t));

	node->writebuf->current_size = 4;

	sendto(node->socket, node->writebuf->data, node->writebuf->current_size, 0, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen);
}

int tftp_send_file(tftp_t *node, char *filename)
{
	int fd = -1;
	
	struct stat fileinfo;

	uint16_t block = 1;
	int nwrite = 0;
	int totalwrite = 0;
	socklen_t sizes;

	fd = open(filename, O_RDONLY);

	if (fd == -1)
	{
		fprintf(stderr, "open: %s\n", strerror(errno));
		return -1;
	}
	fstat(fd, &fileinfo);

	while(1)
	{
		sizes = sizeof(node->server_addr);
		nwrite = tftp_send_data(node, fd, block);
		if (nwrite < 0)
			return -1;
		
		totalwrite += nwrite - 4;
		nwrite = recvfrom(node->socket, node->readbuf->data, 512, 0, (struct sockaddr *) &node->server_addr, &sizes);
		if (node->readbuf->data[1] == TFTP_ACK)
		{

			uint16_t recvblock = *(uint16_t *)&node->readbuf->data[2];
			if (ntohs(recvblock) == block)
			{
				block++;
			}
			else
			{
				sendto(node->socket, node->writebuf->data, node->writebuf->current_size, 0, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen);
				
			}
			
		}

		if (totalwrite >= fileinfo.st_size)
			break;
	}
	close(fd);
	printf("send %d bytes from %s\n", totalwrite, filename);
}

int tftp_recv_file(tftp_t *node, char *filename)
{
	if (node->writebuf->max_size < 516)
		return -1;
	int fd = -1;

	uint16_t block = 1;
	int nread = 0;
	int totalread = 0;
	socklen_t their_addr_size;

	fd = open(filename, O_WRONLY | O_CREAT, 0666);
	if (fd == -1)
	{
		fprintf(stderr, "open: %s\n", strerror(errno));
		return -1;
	}

	while(1)
	{
		their_addr_size = sizeof(node->server_addr);

		nread = recvfrom(node->socket, node->readbuf->data, 512, 0, (struct sockaddr *) &node->server_addr, &their_addr_size);

		if (nread == -1)
		{
			fprintf(stderr, "read: %s\n", strerror(errno));
			return -1;
		}
		else if (nread == 0)
		{
			fprintf(stdout, "Server shut down\n");
			return -1;
		}

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

		write(fd, &node->readbuf->data[4], nread - 4);
		totalread += nread;

		if (nread < 512)
			break;

	}
	close(fd);

	printf("%d bytes has been written to %s\n", totalread, filename);
}

