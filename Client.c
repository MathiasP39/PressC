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
// File management
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>


int static dS;

char* serveurIP;

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


// FILE MANAGEMENT


// Define a structure to hold the arguments
struct file_reception_args {
    int dS;
    char* filename;
};


/**
 * Function to receive a asked file from the server.
*/
void* file_reception(void* args) {
    struct file_reception_args* actual_args = (struct file_reception_args*)args;
    char buffer[1024];
    char file_name[256];
    sprintf(file_name, "./files/%s", actual_args->filename);

    int file_fd = open(file_name, O_WRONLY | O_CREAT, 0666); // Create a new file

    if (file_fd == -1) {
        perror("Failed to open file");
        return NULL;
    }

    ssize_t bytes_received;
    while ((bytes_received = recv(actual_args->dS, buffer, sizeof(buffer), 0)) > 0) {
        write(file_fd, buffer, bytes_received);
    }

    if (bytes_received == -1) {
        perror("Failed to receive file");
        close(file_fd);
        return NULL;
    }

    close(file_fd);
    return (void*)1;
}

/**
 * Function to create a thread for file receiving.
 * This function will be called in message_reception when a specific notification is received from the server.
 * 
 * @param dS The socket descriptor to receive the file from.
 * @return Returns 1 if the thread is successfully created, or -1 if an error occurs.
*/
int file_reception_thread(int dS, char* filename) {
    pthread_t tid;
    struct file_reception_args file_args;
    file_args.dS = dS;
    file_args.filename = filename;
    int i = pthread_create(&tid, NULL, file_reception, &file_args);
    if (i != 0) {
        perror("Error creating thread");
        return -1;
    }

    // Wait for the thread to finish
    void* result;
    if (pthread_join(tid, &result) != 0) {
        perror("Error joining the thread");
        return -1;
    }

    // Check the result of file_reception
    if (result == NULL) {
        perror("Error in file_reception");
        return -1;
    }

    return 1;
}

/**
 * Function to detect the server's notification to receive a file.
 * Notification is under this format : "sprintf(notification, "/receiving %s on %d", filename, file_socket.port);" in the server.
 * Will start the thread for file reception if the notification is detected.
 * 
 * @param message The message received from the server.
 * @return Returns 1 if the notification is detected, 0 if not, or -1 if an error occurs.
*/
int detect_file_reception(char* message) {
    char* token = strtok(message, " ");
    if (strcmp(token, "/receiving") == 0) {
        token = strtok(NULL, " ");
        char* filename = token;
        token = strtok(NULL, " ");
        char* port = token;

        // Create a new socket to receive the file
        int file_socket = socket(PF_INET, SOCK_STREAM, 0);
        if (file_socket == -1) {
            perror("Failed to create file socket");
            return -1;
        }

        // Connect to the server
        if (socket_connection(serveurIP, port, file_socket) != 0) {
            perror("Failed to connect to the server");
            return -1;
        }

        return file_reception_thread(file_socket, filename);
    }
    return 0;
}


// END OF FILE MANAGEMENT


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
        int checkFile = detect_file_reception(message);
        if (checkFile == 1) {
            perror("File received");
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


/**
 * Function to perform an extinction process.
 * This function disconnects the client and terminates the program.
 */
void extinction() {
    puts("You will be disconnected ...");
    close(dS);
    sleep(1);
    exit(0);
}


// MAIN FUNCTION

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

    serveurIP = argv[1];

    pthread_t tid;
    pthread_t tid2;

    pthread_t tidfilesnd; //Thread for file sending

    printf("Pseudo : ");

    int i = pthread_create (&tid, NULL, message_reception,&dS);
    int j = pthread_create(&tid2,NULL,message_sending,&dS);

    if (pthread_join(tid,NULL) == 0 || pthread_join(tid2,NULL) == 0) {
        extinction();
    }
}