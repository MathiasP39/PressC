#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
This program generates client for communication
Launch it with command : ./Client ip port type (type is 0 for send first and 1 for receive first)
*/

int main(int argc, char *argv[]) {

    if (argc =! 4) {
        perror("Nombre d'argument incorrect");
        exit(0);
    }
    printf("Program launched\n");

    int type = atoi(argv[3]);

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    printf("Socket Created\n");

    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET,argv[1],&(aS.sin_addr));
    aS.sin_port = htons(atoi(argv[2]));
    socklen_t lgA = sizeof(struct sockaddr_in);
    connect(dS, (struct sockaddr *) &aS, lgA);
    printf("Socket Connected\n");

    bool running = true;

    while (running){
        switch (type) {
            case 0:
                type = 1;
            case 1:
                type = 0;
        }
    }
    
}