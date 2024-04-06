#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


//Command to launch this program : ./Serveur port 

int main(int argc, char *argv[]) {

    if (argc != 2) { //security : check if the number of arguments is correct
        //printf("Number of arguments incorrect\n");
        perror("Incorrect number of arguments");
        printf("Usage : %s <port>\n", argv[0]);
        exit(0);
        return -1;
    }

    printf("Start program\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0); //keep the socket descriptor (id of the socket)
    printf("Socket created\n");

    struct sockaddr_in ad;
    ad.sin_family = AF_INET; //address family
    ad.sin_addr.s_addr = INADDR_ANY; //address to accept any incoming messages
    ad.sin_port = htons(atoi(argv[1])); //port passed as argument
    int connect = bind(dS, (struct sockaddr*)&ad, sizeof(ad)); //variable "connect" to avoid conflict with the function connect
    if (connect < 0) {
        printf("Connection error: bind failed\n");
        return -1;
    }
    printf("Socket named\n");

    int ecoute = listen(dS, 7); //variable "ecoute" to avoid conflict with the function listen
    if (ecoute < 0) {
        printf("Connection error: listen failed\n");
        return -1;
    }
    printf("Listening mode\n");

    struct sockaddr_in aC ;
    socklen_t lenght = sizeof(struct sockaddr_in); //keep the length of the address
    int dSC = accept(dS, (struct sockaddr*) &aC, &lenght); //keep the (new) socket descriptor of the client
    if (dSC == -1) {
        printf("Connection error: client failed to connect\n");
        return -1;
    }
    printf("\n--Client connected--\n\n");
    
    char msg [300]; //message buffer : 300 characters max

    int rec = recv(dSC, msg, sizeof(msg), 0);
    msg[rec] = '\0';
    printf("Message received : %s\n", msg);


}