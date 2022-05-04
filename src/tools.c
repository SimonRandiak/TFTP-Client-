#include "../include/tools.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>


int udp_send_data(int socket, const void *data, const size_t data_len, struct sockaddr *to_addr, socklen_t to_addr_size)
{

	int nwrite = sendto(socket, data, data_len, 0, to_addr, to_addr_size);
	if (nwrite < 0)
	{
#if defined _WIN32
		fprintf(stderr, "sendto: %d\n", WSAGetLastError());
#elif defined __linux__
		fprintf(stderr, "sendto: %s\n", strerror(errno));
#endif
	}
	return nwrite;
}

int udp_recv_data(int socket, void *data, const size_t data_len, struct sockaddr *to_addr, socklen_t *to_addr_size)
{

	int nread = recvfrom(socket, data, data_len, 0, to_addr, to_addr_size);
	if (nread < 0)
	{
#if defined _WIN32
			fprintf(stderr, "recvfrom: %d\n", WSAGetLastError());
#elif defined __linux__
			fprintf(stderr, "recvfrom: %s\n", strerror(errno));
#endif
		return -1; 
	}
	return nread;
}

int udp_recv_all_data(int socket, void *data, const size_t data_len, struct sockaddr *to_addr, socklen_t *to_addr_size)
{
	int nread = 0;
	int totalread = 0;

	while(totalread < data_len)
	{
		nread = recvfrom(socket, data, data_len, 0, to_addr, to_addr_size);
		if (nread < 0)
		{
#if defined _WIN32
			fprintf(stderr, "recvfrom: %d\n", WSAGetLastError());
#elif defined __linux__
			fprintf(stderr, "recvfrom: %s\n", strerror(errno));
#endif
			return -1;
		}
		totalread += nread;
	}
	return totalread;
	
}

int udp_send_all_data(int socket, const void *data, const size_t data_len, struct sockaddr *to_addr, socklen_t to_addr_size)
{
	int nread = 0;
	int totalread = 0;

	while(totalread < data_len)
	{
		nread = sendto(socket, data, data_len, 0, to_addr, to_addr_size);
		if (nread < 0)
		{
#if defined _WIN32
		fprintf(stderr, "sendto: %d\n", WSAGetLastError());
#elif defined __linux__
		fprintf(stderr, "sendto: %s\n", strerror(errno));
#endif
			return -1;
		}
		totalread += nread;
	}
	return totalread;
}
