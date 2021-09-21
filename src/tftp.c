#include "../include/tftp.h"
#include "../include/tools.h"
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
				char *filename = buf->data+4;

				tftp_send_write_request(&node, filename, tftp_transfer_modes[0]);
				tftp_send_file(&node, filename);	
			}
			else if  ( strcasecmp(strtok(buf->data, " "), tftp_commands[1]) == 0)
			{
				char *filename = buf->data+4;
				
				tftp_send_read_request(&node, filename, tftp_transfer_modes[0]);
				tftp_recv_file(&node, filename);
			}
			else if ( strcasecmp(strtok(buf->data, " "), tftp_commands[2]) == 0)
			{
				fprintf(stderr, "tftp: closing\n"); 
				break;	
			}
			else if ( strcasecmp(strtok(buf->data, " "), tftp_commands[3]) == 0)
			{
				fprintf(stderr, "tftp: not implemented\n");
			}
			else
			{
				fprintf(stdout, "tftp: Invalid Command: %s\n", strtok(buf->data, " "));	
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
	node->server_addr_size = sizeof (struct sockaddr_storage);
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

int tftp_send_read_request(tftp_t *node, const char *filename, const char *mode)
{
	size_t packet_len = strlen(filename) + strlen(mode) + 4;

	memset(node->writebuf->data,0, packet_len);

	node->writebuf->data[1] = TFTP_RRQ;

	memcpy(node->writebuf->data + 2, filename, strlen(filename));

	memcpy(node->writebuf->data + 3 + strlen(filename), mode, strlen(mode));

	udp_send_all_data(node->socket, node->writebuf->data, packet_len, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen);

}

int tftp_send_write_request(tftp_t *node, const char *filename, const char *mode)
{
	size_t packet_len = strlen(filename) + strlen(mode) + 4;

	memset(node->writebuf->data,0, packet_len);

	node->writebuf->data[1] = TFTP_WRQ;

	memcpy(node->writebuf->data + 2, filename, strlen(filename));

	memcpy(node->writebuf->data + 3 + strlen(filename), mode, strlen(mode));

	udp_send_all_data(node->socket, node->writebuf->data, packet_len, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen);
	
	udp_recv_data(node->socket, node->readbuf->data, node->readbuf->max_size, (struct sockaddr *) &node->server_addr, &node->server_addr_size);

	if ((uint8_t) node->readbuf->data[1] == TFTP_ACK)
	{
		return 0;
	}
	return -1;


}
int tftp_send_data(tftp_t *node, int fd, int block)
{
	int nread = -1;

	buffer_clear(node->writebuf);

	node->writebuf->data[1] = TFTP_DATA;
	block = htons(block);

	memcpy(&node->writebuf->data[2], &block, sizeof(uint16_t));

	nread = read(fd, &node->writebuf->data[4], 512);
	
	if (nread < 0)
	{
		fprintf(stderr, "read: %s\n", strerror(errno));
		return -1;
	}

	udp_send_all_data(node->socket, node->writebuf->data, nread + 4, (struct sockaddr *) &node->server_addr, node->server_addr_size);
}

int tftp_send_ack(tftp_t *node, uint16_t block)
{

	buffer_clear(node->writebuf);
	node->writebuf->data[1] = TFTP_ACK;

	block = htons(block);

	memcpy(&node->writebuf->data[2], &block, sizeof(uint16_t));

	node->writebuf->current_size = 4;

	udp_send_all_data(node->socket, node->writebuf->data, 4, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen);
}

int tftp_send_file(tftp_t *node, char *filename)
{
	
	int fd = -1;
	
	struct stat fileinfo;

	int nwrite = 0;
	unsigned int totalread = 0;
	uint16_t block = 1;

	fd = open(filename, O_RDONLY);

	if (fd == -1)
	{
		fprintf(stderr, "open: %s\n", strerror(errno));
		return -1;
	}
	fstat(fd, &fileinfo);

	while(1)
	{

		nwrite = tftp_send_data(node, fd, block);
		if (nwrite == -1)
			return -1;
		totalread += nwrite - 4;
	
		
		nwrite = udp_recv_data(node->socket, node->writebuf->data, node->writebuf->max_size, (struct sockaddr *) &node->server_addr, &node->server_addr_size);	
		if (node->readbuf->data[1] == TFTP_ACK)
		{

			uint16_t recvblock = *(uint16_t *)&node->readbuf->data[2];
			if (ntohs(recvblock) == block)
			{
				block++;
			}
			else
			{
				udp_send_data(node->socket, node->writebuf->data, node->writebuf->current_size, node->ConnectNode->ai_addr, node->ConnectNode->ai_addrlen);
				
			}
			
		}
			if (totalread >= fileinfo.st_size)
				break;
	}
	close(fd);
}

int tftp_recv_file(tftp_t *node, char *filename)
{
	int fd = -1;

	uint16_t block = 1;
	int nread = 0;
	int totalread = 0;


	fd = open(filename, O_WRONLY | O_CREAT, 0666);
	if (fd == -1)
	{
		fprintf(stderr, "open: %s\n", strerror(errno));
		return -1;
	}

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

		write(fd, &node->readbuf->data[4], nread - 4);
		totalread += nread;

		if ((nread - 4) < 512)
			break;

	}
	close(fd);

	printf("%d bytes has been written to %s\n", totalread, filename);
}

