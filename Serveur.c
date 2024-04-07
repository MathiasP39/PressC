#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>


//Command to launch this program : ./Serveur port1 port2
/*
This program should act as a relay of message 
This should start the conversation when 2 Clients are connected, capture the senders and the receiver and switch them, and end when "fin" is sended by someone
*/

int main(int argc, char *argv[]) {

    if (argc != 2) { //security : check if the number of arguments is correct
        //printf("Number of arguments incorrect\n");
        perror("Incorrect number of arguments");
        printf("Usage : %s <port>\n", argv[0]);
        exit(0);
        return -1;
    }

    printf("Start program\n");

    bool running = true;

    /*
    One of the problem is to find who is to konow who is the sender and who is the receiver for the start, given that this is alternante next
    The idea here is that the both clients send their type to be affected the correct role
    */

   //This loop should be running unless there is an external interruption, because this allow to handle the connection of new Clients when the last conversation has ended
    while (running) {
        int dS1 = socket(PF_INET, SOCK_STREAM, 0); //keep the socket descriptor (id of the socket)
        printf("Socket 1 created\n");

        int dS2 = socket(PF_INET, SOCK_STREAM, 0); //keep the socket descriptor (id of the socket)
        printf("Socket 2 created\n");

        //Get the adresses of the first Client
        struct sockaddr_in adresse1;
        adresse1.sin_family = AF_INET; //address family
        adresse1.sin_addr.s_addr = INADDR_ANY; //address to accept any incoming messages
        adresse1.sin_port = htons(atoi(argv[1])); //port passed as argument

        struct sockaddr_in adresse2;
        adresse2.sin_family = AF_INET; //address family
        adresse2.sin_addr.s_addr = INADDR_ANY; //address to accept any incoming messages
        adresse2.sin_port = htons(atoi(argv[2])); //port passed as argument

        int connect = bind(dS1, (struct sockaddr*)&adresse1, sizeof(adresse1)); //variable "connect" to avoid conflict with the function connect
        if (connect != 0) {
            printf("Connection error: bind failed\n");
            return connect;
        }
        printf("Socket 1 named\n");
        
        int connect = bind(dS1, (struct sockaddr*)&adresse1, sizeof(adresse1)); //variable "connect" to avoid conflict with the function connect
        if (connect != 0) {
            printf("Connection error: bind failed\n");
            return connect;
        }
        printf("Socket 2 named\n");

        //Get ready for receiving the type of the client
        int ecoute = listen(dS1,1);
        if (ecoute < 0) {
            perror("Connection error: listen failed\n");
            return ecoute;
        }
        printf("Listening mode\n");

        int ecoute = listen(dS2,1);
        if (ecoute < 0) {
            perror("Connection error: listen failed\n");
            return ecoute;
        }
        printf("Listening mode\n");

        struct sockaddr_in aC ;
        socklen_t lenght = sizeof(struct sockaddr_in); //keep the length of the address
        int dSClient1 = accept(dS1, (struct sockaddr*) &aC, &lenght); //keep the (new) socket descriptor of the client
        if (dSClient1 == -1) {
            perror("Connection error: client failed to connect\n");
            return -1;
        }
        printf("\n--Client connected--\n\n");


        
    }

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