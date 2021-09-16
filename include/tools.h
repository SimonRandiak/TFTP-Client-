#ifndef TOOLS_H
#define TOOLS_H

#include "tftp.h"

int recv_data(tftp_t *node);

int send_data(tftp_t *node);

#endif
