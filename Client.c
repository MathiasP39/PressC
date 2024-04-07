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

    //Check of number of argument
    if (argc =! 4) {
        perror("Incorrect number of aruments");
        exit(0);
    }
    printf("Program launched\n");

    //Socket creation
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    printf("Socket Created\n");

    //Socket connection
    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET,argv[1],&(aS.sin_addr));
    aS.sin_port = htons(atoi(argv[2]));
    socklen_t lgA = sizeof(struct sockaddr_in);
    connect(dS, (struct sockaddr *) &aS, lgA);
    printf("Socket Connected\n");


    bool running = true;
    int type = atoi(argv[3]);

    send(dS,&type,sizeof(int),0);


    char * message = (char *)malloc(sizeof(char) * 301); //Allocation of space for the message 
    /*
    Here is the loop that run the program,  should be interrupted when the senders type "fin"
    To add : Typing of message in console and end of chat with "fin" word
    */
    while (running){
        switch (type) {
            //Sender Case
            case 0:
                //char message[300] = "Hello the pc this is the earth" ; // Message creation
                send(dS, message, 300 , 0) ; // Sending of message to server 
                printf("Le message envoye est %s \n", message); //Check of what is the message send
                printf("Message Envoyé \n"); //Confirm of sending
                //free(message);
                type = 1; //Switch type of client
                break;
            //Receiver Case
            case 1:
                //char * message = (char *)malloc(sizeof(char) * 301); //Allocation of space for the message 
                recv(dS, message, 300, 0); //Reception of message
                printf("%s",message);
                //free(message);
                type = 0;//Switch type of client
                break;
        }
    }
    
}