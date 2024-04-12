#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>


//Command to launch this program : ./Serveur port1 port2
// Exemple : ./Serveur 3500 3501
/*
This program should act as a relay of message 
This should start the conversation when 2 Clients are connected, capture the senders and the receiver and switch them, and end when "fin" is sended by someone
*/

int creation_socket() {
        int dS = socket(PF_INET, SOCK_STREAM, 0);
        printf("Socket created \n");
        return dS;
}

struct sockaddr_in param_socket_adresse(char * port) {
    //Get the adresses of the first Client
    struct sockaddr_in adresse1;
    adresse1.sin_family = AF_INET; //address family
    adresse1.sin_addr.s_addr = INADDR_ANY; //address to accept any incoming messages
    adresse1.sin_port = htons(atoi(port)); //port passed as argument
    printf("Adresse creation");
    return adresse1;
}

int make_bind(int socket,struct sockaddr_in adresse) {
    int connect = bind(socket,(struct sockaddr*)&adresse,sizeof(adresse)); //variable "connect" to avoid conflict with the function connect
    if (connect != 0) {
        printf("Connection error: bind failed\n");
        return connect;
    }
    printf("Socket 1 named\n");
}

int main(int argc, char *argv[]) {

    if (argc != 3) { //security : check if the number of arguments is correct
        //printf("Number of arguments incorrect\n");
        perror("Incorrect number of arguments");
        printf("Usage : %s <port>\n", argv[0]);
        exit(0);
        return -1;
    }

    printf("Start program\n");

    bool running = true;    

    int dS1 = creation_socket();

    int dS2 = creation_socket();//keep the socket descriptor (id of the socket)

    struct sockaddr_in adresse1 = param_socket_adresse(argv[1]);

    struct sockaddr_in adresse2 = param_socket_adresse(argv[2]);

    int connect1 = make_bind(dS1,adresse1);

    int connect2 = make_bind(dS2,adresse2);
    /*
    One of the problem is to find who is to konow who is the sender and who is the receiver for the start, given that this is alternante next
    The idea here is that the both clients send their type to be affected the correct role
    */
   //The part above is 
   //This loop should be running unless there is an external interruption, because this allow to handle the connection of new Clients when the last conversation has ended
    while (running) {

        //Get ready for receiving the type of the client
        int ecoute = listen(dS1,1);
        if (ecoute < 0) {
            perror("Connection error: listen failed\n");
            return ecoute;
        }
        printf("Listening mode\n");

        int ecoute1 = listen(dS2,1);
        if (ecoute1 < 0) {
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
        printf("\n-- Client connected 1 --\n\n");

        struct sockaddr_in aD ;
        socklen_t lenght1 = sizeof(struct sockaddr_in); //keep the length of the address
        int dSClient2 = accept(dS2, (struct sockaddr*) &aD, &lenght1); //keep the (new) socket descriptor of the client
        if (dSClient2 == -1) {
            perror("Connection error: client failed to connect\n");
            return -1;
        }
        printf("\n-- Client 2 connected --\n\n");

        int type_of_client;

        int res = recv(dSClient1, &type_of_client, sizeof(int), 0);

        int * dSClientReceiver = (int *)malloc(sizeof(int));

        int * dSClientSender = (int *)malloc(sizeof(int));

        printf("Le type recu est : %d \n",type_of_client);

        //There is a problem there, when wyou execute, it sends the error below
        if (res < 0) {
            perror("Reception failed");
            exit(-1);
        }

        if (type_of_client == 1) {
            dSClientReceiver = &dSClient1;

            int res = recv(dSClient2, &type_of_client, sizeof(int), 0);

            if (res < 0) {
                perror("Reception failed");
                exit(-1);
            }

            if (type_of_client != 0) {
                perror("You should have a sender and a receiver, the 2 clients have the same function in this situation");
                exit(-1);
            }
            else {
                dSClientSender = &dSClient2;
            }
        }
        else {
            dSClientSender = &dSClient1;

            int res = recv(dSClient2, &type_of_client, sizeof(int), 0);

            if (res < 0) {
                perror("Reception failed");
                exit(res);
            }

            if (type_of_client != 1) {
                perror("You should have a sender and a receiver, the 2 clients have the same function in this situation");
                exit(-1);
            }
            else {
                dSClientReceiver = &dSClient2;
            }
        }

        printf("Initialisation réussi \n") ;
        
        bool conversation = true;
        char message[300];
        while (conversation) {

            int res =  recv(*dSClientSender, &message, sizeof(char)*300, 0);
            if (res < 0) {
                perror("Error receiving the message");
                exit(0);
            }

            res = send(*dSClientReceiver, message, sizeof(char)*300 , 0);
            if (res < 0) {
                perror("Error receiving the message");
                exit(0);
            }

            if (strcmp(message,"fin")) {
                int fermeture;
                fermeture = close(dSClient1);
                if (fermeture < 0) {
                    perror("Error when closing dSClient1");
                }
                fermeture = close(dSClient2);
                if (fermeture < 0) {
                    perror("Error when closing dSClient2");
                }
                printf("fin de fermeture \n");
                conversation = false;
            }
            else {
                int *dSechange = dSClientReceiver;
                dSClientReceiver = dSClientSender;
                dSClientSender = dSechange; 
            }
        }
        
    }

}