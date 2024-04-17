#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

/*
This program generates client for communication
Launch it with command : ./Client ip port
//Example : Launch that first : ./Client 127.0.0.1 3500 and then ./Client 127.0.0.1 3500
*/

char* remove_backslash (char* word) {
    if (word[strlen(word)-1] == '\n') {
        word[strlen(word)-1] = '\0';
    }
    return word;
}

void* message_reception (void * args) {
    int * dS = (int*) args;
    printf("dS vaut : %d \n",*dS);
    int running = 1;
    char * message = (char *)malloc(sizeof(char) * 301);
    while (running) {
        int checkReceive = recv(*dS, message, 300, 0); //Reception of message
        if (checkReceive == -1){
            perror("Receive failed");
            exit(0);
        }
        puts(message);
        sleep(1);
    }
    pthread_exit(0); 
}

void* message_sending (void * args) {
    //printf("Entrée dans la fonction d'envoie de message");
    int* dS = (int*) args;
    printf("dS vaut : %d \n",*dS);
    char * message = (char *)malloc(sizeof(char) * 301);
    int running = 1;
    while (running) {
        fgets(message,300,stdin);
        message = remove_backslash(message);
        int checkSend = send(*dS, message, 300 , 0); // Sending of message to server
        if (checkSend == -1){
            perror("Send failed");
            exit(0);
        }
        else {
            puts("le message enoyé est : ");
            puts(message);
        }
        sleep(1);
    }
    pthread_exit(0);
}

int main(int argc, char *argv[]) {

    //Check of number of argument
    if (argc =! 3) {
        perror("Incorrect number of arguments");
        exit(0);
    }
    //printf("Program launched\n");

    //Socket creation
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1){
        perror("Socket creation failed");
        exit(0);
    }
    //printf("Socket Created\n");

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

    pthread_t tid;
    pthread_t tid2;

    printf("dS vaut : %d \n",dS);

    int i = pthread_create (&tid, NULL, message_reception,&dS);
    int j = pthread_create(&tid2,NULL,message_sending,&dS);

    pthread_join(tid,NULL);
    pthread_join(tid2,NULL);


    int checkSD = close(dS);
    if (checkSD == -1){
        perror("Shutdown failed");
        exit(0);
    }
}