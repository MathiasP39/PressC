#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
This program generates client for communication
Launch it with command : ./Client ip port type (type is 0 for send first and 1 for receive first)
//Example : ./Client 127.0.0.1 3500 0 
// ./Client 127.0.0.1 3501 1 
*/

int main(int argc, char *argv[]) {

    //Check of number of argument
    if (argc =! 4) {
        perror("Incorrect number of arguments");
        exit(0);
    }
    printf("Program launched\n");


    //Initialization of client type
    int type = atoi(argv[3]);
    if (type != 0 && type != 1){
        perror("Type of client not recognized");
        exit(0);
    }


    //Socket creation
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1){
        perror("Socket creation failed");
        exit(0);
    }
    printf("Socket Created\n");

    //Socket connection
    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET,argv[1],&(aS.sin_addr));
    aS.sin_port = htons(atoi(argv[2]));
    socklen_t lgA = sizeof(struct sockaddr_in);
    int checkConnect = connect(dS, (struct sockaddr *) &aS, lgA);
    if (checkConnect == -1){
        perror("Connection failed");
        exit(0);
    }
    printf("Socket Connected\n");


    bool running = true;


    int checkSendType = send(dS,&type,sizeof(int),0);
    if (checkSendType == -1){
        perror("Send type failed");
        exit(0);
    }


    char * message = (char *)malloc(sizeof(char) * 301); //Allocation of space for the message 
    /*
    Here is the loop that run the program,  should be interrupted when the senders type "fin"
    To add : Typing of message in console and end of chat with "fin" word
    */
    while (running){
        switch (type) {
            //Sender Case
            case 0:
                message = malloc(300 * sizeof(char));
                //fgets(message,300,stdin);
                message = "Bonjour chef";
                int checkSend = send(dS, message, 300 , 0) ; // Sending of message to server
                if (checkSend == -1){
                    perror("Send failed");
                    exit(0);
                }
                printf("Le message envoye est %s \n", message); //Check of what is the message send
<<<<<<< HEAD
                printf("Message Envoyé \n"); //Confirm of sending
                break;
            //Receiver Case
            case 1:
                recv(dS, message, 300, 0); //Reception of message
=======
                printf("Message envoyé \n"); //Confirm of sending
                //free(message);
                //type = 1; //Switch type of client
                break;
            //Receiver Case
            case 1:
                //char * message = (char *)malloc(sizeof(char) * 301); //Allocation of space for the message 
                int checkReceive = recv(dS, message, 300, 0); //Reception of message
                if (checkReceive == -1){
                    perror("Receive failed");
                    exit(0);
                }
>>>>>>> af9e3d8 (Error management for client)
                printf("Message recu : %s \n",message);
                break;
        }

        if (type == 1){
            type = 0;
        }
        else {
            type = 1;
        }
    }
    int checkSD = shutdown(dS,2);
    if (checkSD == -1){
        perror("Shutdown failed");
        exit(0);
    }
}