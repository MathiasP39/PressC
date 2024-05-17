#ifndef ANY_UNIQUE_NAME_HERE
#define ANY_UNIQUE_NAME_HERE

#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int send_message (int descripteur, char* message);

int recv_message(int descripteur,char ** message);

char* update_file_list(const char* directory);


#endif