#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../include/buffer.h"
#include "../include/tftp.h"

int main(int argc, char *argv[])
{
	tftp_loop(argc, argv);
}
