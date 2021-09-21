#include "../include/buffer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

buffer_t *buffer_allocate(unsigned int alloc_size)
{
	buffer_t *buf;
	buf = malloc(sizeof(buffer_t));
	buf->max_size = alloc_size;
	buf->current_size = 0;
	buf->data = (unsigned char *) calloc(1, sizeof(unsigned char) * alloc_size);
	return buf;
}

int buffer_release(buffer_t *buf)
{
	if (buf->data)
	{
		free(buf->data);
		free(buf);
		return 0;
	}
		return -1;
}

void buffer_clear(buffer_t *buf)
{
	memset(buf->data, 0, buf->current_size);
	buf->current_size = 0;

}
int get_user_input(buffer_t *buf)
{
	buffer_clear(buf);
	int c;
	int charcounter = 0;

	while((c = getchar()) != EOF && c != '\n' && charcounter != buf->max_size)
	{
		buf->data[charcounter] = c;
		charcounter++;
	}
	buf->current_size = charcounter;
	buf->data[charcounter] = '\0';	
}
