#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

/*
This program generates client for communication
Launch it with command : ./Client ip port type
*/

int main(int argc, char *argv[]) {
    printf("Début programme\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    printf("Socket Créé\n");

    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ;
    aS.sin_port = htons(atoi(argv[2])) ;
    socklen_t lgA = sizeof(struct sockaddr_in) ;
    connect(dS, (struct sockaddr *) &aS, lgA) ;
    printf("Socket Connecté\n");

    
}