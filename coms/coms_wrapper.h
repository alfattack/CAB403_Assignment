#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include "commands.h"

#define CLOSED -2

int send_sock(int sock_fd, void * buf, int size);
int rec_sock(int sock_fd, void * buf, int size);
