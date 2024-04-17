#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


//Command to launch this program : ./Serveur port
// Exemple : ./Serveur 3500
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
    printf("Adresse creation \n");
    return adresse1;
}

int make_bind(int socket,struct sockaddr_in adresse) {
    int connect = bind(socket,(struct sockaddr*)&adresse,sizeof(adresse)); //variable "connect" to avoid conflict with the function connect
    if (connect != 0) {
        printf("Connection error: bind failed\n");
        return connect;
    }
    printf("Socket named\n");
}

int main(int argc, char *argv[]) {

    if (argc != 2) { //security : check if the number of arguments is correct
        //printf("Number of arguments incorrect\n");
        perror("Incorrect number of arguments");
        printf("Usage : %s <port>\n", argv[0]);
        exit(0);
        return -1;
    }

    printf("Start program\n");

    short running = 1;    

    int dS = creation_socket();

    struct sockaddr_in adresse = param_socket_adresse(argv[1]);

    int connect = make_bind(dS,adresse);

    /*
    One of the problem is to find who is to konow who is the sender and who is the receiver for the start, given that this is alternante next
    The idea here is that the both clients send their type to be affected the correct role
    */
   //The part above is 
   //This loop should be running unless there is an external interruption, because this allow to handle the connection of new Clients when the last conversation has ended
    while (running) {

        //Get ready for receiving the type of the client
        int ecoute = listen(dS,1);
        if (ecoute < 0) {
            perror("Connection error: listen failed\n");
            return ecoute;
        }
        printf("Listening mode\n");

        struct sockaddr_in aC ;
        socklen_t lenght = sizeof(struct sockaddr_in); //keep the length of the address
        int dSClient1 = accept(dS, (struct sockaddr*) &aC, &lenght); //keep the (new) socket descriptor of the client
        if (dSClient1 == -1) {
            perror("Connection error: client failed to connect\n");
            return -1;
        }
        printf("\n-- Client connected 1 --\n\n");

        struct sockaddr_in aD ;
        socklen_t lenght1 = sizeof(struct sockaddr_in); //keep the length of the address
        int dSClient2 = accept(dS, (struct sockaddr*) &aD, &lenght1); //keep the (new) socket descriptor of the client
        if (dSClient2 == -1) {
            perror("Connection error: client failed to connect\n");
            return -1;
        }
        printf("\n-- Client 2 connected --\n\n");

        int dSClientReceiver = dSClient2;

        int dSClientSender = dSClient1;

        printf("Initialisation réussi \n") ;
        short conversation = 1;
        char message[300];
        while (conversation) {
            printf("En attente d'un message \n");
            int res =  recv(dSClientSender, message, sizeof(char)*300, 0);
            printf("message recu : %s",message);
            if (res < 0) {
                perror("Error receiving the message");
                exit(0);
            }

            res = send(dSClientReceiver, message, sizeof(char)*300 , 0);
            if (res < 0) {
                perror("Error receiving the message");
                exit(0);
            }

            if (strcmp(message,"fin")==0) {
                int fermeture;
                fermeture = close(dSClientSender);
                if (fermeture < 0) {
                    perror("Error when closing dSClient1");
                }
                fermeture = close(dSClient2);
                if (fermeture < 0) {
                    perror("Error when closing dSClient2");
                }
                printf("Conversation fermé\n");
                conversation = 0;
            }
            else {
                int dSechange = dSClientReceiver;
                dSClientReceiver = dSClientSender;
                dSClientSender = dSechange; 
            }

            sleep(1);
        }
        
    }

}