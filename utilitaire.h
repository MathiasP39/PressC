#ifndef ANY_UNIQUE_NAME_HERE
#define ANY_UNIQUE_NAME_HERE

#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const int NB_CLIENT_MAX = 10;

int send_message (int descripteur, char* message);

int recv_message(int descripteur,char ** message);


#endif