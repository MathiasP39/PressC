#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "utilitaire.h"
#include <signal.h>
#include <dirent.h>


int static dS;

/*
This program generates client for communication
Launch it with command : ./Client ip port
//Example : Launch : ./Client 127.0.0.1 3500 for each client 
*/

/*
Function that removes the backslash n at the end of a string (char[])
Take the pointer to the first character
Return the word without the characyer or just the word if there isn't
Use here to remove it from the fget input
*/
char* remove_backslash (char* word) {
    if (word[strlen(word)-1] == '\n') {
        word[strlen(word)-1] = '\0';
    }
    return word;
}

/*
Function that is used to recep message from another client coming through the server
Supposed to be used in a thread unless you just want to receip
Needs the id of the socket to the server in argument
Return 0 if all went good and -1 if there was an error
*/
void* message_reception (void * args) {
    int * dS = (int*) args;
    int running = 1;
    char * message;
    while (running) {
        int checkReceive = recv_message(*dS, &message); //Reception of message
        if (checkReceive < 0) {
            perror("Error receiving message");
        }
        else if (checkReceive == 0) {
            puts("Connection disconnected");
            pthread_exit(0);
        }
        else {
            puts(message);
        }
        sleep(0.1);
    }
    pthread_exit(0);
}

/*
Function that is used to ensure the sending of message to the server
Supposed to be used in a thread
The id of the socket used is required in argument 
Return 0 if it succeed, -1 if an error occured
*/
void* message_sending (void * args) {
    int* dS = (int*) args;
    char * message = (char *)malloc(sizeof(char) * 301);
    int running = 1;
    while (running) {
        fgets(message,300,stdin);
        message = remove_backslash(message);
        int checkSend = send_message(*dS, message); // Sending of message to server
        if (checkSend == -1){
            perror("Send failed");
            exit(EXIT_FAILURE);
        }
        if (strcmp(message,"fin") == 0) {
            puts("Deconnexion de la discussion");
            running = 0;
        }
        sleep(0.1);
    }
    pthread_exit(0);
}

/*
Function that connect the socket to the other which is located at the ip and port 
You need to give the ip, the port and the id
Return 0 if all went good 
*/
int socket_connection (char * ip, char* port,int socket_id) {
    //Socket connection
    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&(aS.sin_addr));
    aS.sin_port = htons(atoi(port));
    socklen_t lgA = sizeof(struct sockaddr_in);
    int checkConnect = connect(socket_id, (struct sockaddr *) &aS, lgA);
    if (checkConnect == -1){
        perror("Connection failed");
        exit(-1);
    }
    return 0;
}

void extinction() {
    puts("You will be disconnected ...");
    close(dS);
    sleep(1);
    exit(0);
}

int main(int argc, char *argv[]) {

    signal(SIGINT, extinction);

    //Check of number of argument
    if (argc =! 3) {
        perror("Incorrect number of arguments");
        exit(0);
    }

    //Socket creation
    dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1){
        perror("Socket creation failed");
        exit(0);
    }

    //Socket connection
    socket_connection(argv[1],argv[2],dS);

    pthread_t tid;
    pthread_t tid2;

    pthread_t tidfilesnd; //Thread for file sending
    pthread_t tidfilercv; //Thread for file receiving

    printf("Pseudo : ");

    int i = pthread_create (&tid, NULL, message_reception,&dS);
    int j = pthread_create(&tid2,NULL,message_sending,&dS);

    if (pthread_join(tid,NULL) == 0 || pthread_join(tid2,NULL) == 0) {
        extinction();
    }
}