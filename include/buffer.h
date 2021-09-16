#ifndef BUFFER_H
#define BUFFER_H

typedef struct
{
	unsigned char *data;
	unsigned int max_size;
	unsigned int current_size;
} buffer_t;

buffer_t *buffer_allocate(unsigned int alloc_size);

int buffer_release(buffer_t *buf);

void buffer_clear(buffer_t *buf);

int get_user_input(buffer_t *buf);

#endif
