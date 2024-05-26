#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "ServeurConnection.h"
#include "ressources.h"
#include "client.h"
#include "../lib_headers/utils.h"

int static server_running = 1;

/**
 * @struct arg_get_client
 * @brief Structure representing the arguments for getting clients.
 * 
 * This structure holds the necessary information for getting clients in the server.
 * It includes an array of client IDs, the maximum number of clients, and the server socket descriptor.
 */
struct arg_get_client {
    int dS;                /**< Server socket descriptor */
};

/**
 * @struct thread_argument
 * @brief Structure representing the arguments passed to a thread function.
 * 
 * This structure contains the file descriptor of the client connection and an array of client descriptors.
 */
struct thread_argument {
    int descripteur;        /**< The file descriptor of the client connection */
};

/**
 * Handles the conversation between two clients.
 * 
 * This function is responsible for managing the communication between two clients. It receives messages from one client and sends them to the other client.
 * 
 * @param arg The argument structure containing the socket descriptor and the array of client socket descriptors.
 */
void * discussion (void * arg) {
    struct thread_argument * argument = (struct thread_argument *) arg;
    int conversation = 1;
    char *message = NULL; //The message received. Initialized to NULL to avoid recv_message to free a non-allocated memory
    int dS = argument->descripteur;

    char * sender = (char*) malloc(sizeof(char));
    int code = get_nickname( dS, &sender);
    if (code == -1) {
        perror("Error getting the nickname");
    }
    sender[strcspn(sender, " :")] = '\0'; //Remove the " :" from the nickname

    //---The conversation loop---

    while (server_running && conversation) {
        int res =  recv_message(dS, &message);
        if (res <= 0) {
            if (res == 0) {
                int resultat = delete_client(dS);
                if (resultat == 0) {
                puts("Suppression réussie");}
            }

            printf("\033[94m%s\033[0m s'est déconnecté\n", sender);

            conversation = 0;
        }
        else {
            printf("Message recu : %s \n",message);
            char * pseudo = (char*) malloc(sizeof(char));// Variabe to store nickname
            int code = get_nickname(dS,&pseudo); //Here we get the nickname of the sender to add it to the message
            int rep = analyse(message, dS);
            if (rep == 2) { //Case no command
                strcat(pseudo,message);
                res = send_all(dS,pseudo);
                free(pseudo);
            }else if (rep == 0) { //Case command /quit 
                puts("do something");
            }else if (rep == -1){
                puts("Error in the command");
            }
        }
    }
    free(message);
    pthread_exit(0);
}


/**
 * Gets a client connection.
 *
 * This function is responsible for handling the connection of new clients to the server.
 * It continuously accepts new client connections and adds them to the client array.
 * If the maximum number of clients is reached, it sends a message to the client indicating that the server is full.
 * If a client is successfully connected, it creates a new thread to handle the conversation with the client using the "discussion" function.
 *
 * @param arg The argument structure containing the client array, the maximum number of clients, and the server socket descriptor.
 */
void * get_client (void * arg) {
    server_info *args = (server_info *) arg;
    while (server_running) {
        struct sockaddr_in aC ;
        int dSClient = connect_to_client(*args);
        int res = try_lock_sem_nb_client();
        if (res == -1) {
            char message[] = "You can't connect there is already too many people connected, retry later";
            send_message(dSClient, message);
            close(dSClient);
        }
        else if (res == 0) {
            res = add_client(dSClient);
            res = set_nickname(dSClient);
            if (res == -1) {
                perror("set : Error getting the nickname");
                close(dSClient);
            }
            else {
                puts("Client ajouté");
                pthread_t tid;
                struct thread_argument argument = {dSClient};
                int i = pthread_create (&tid, NULL, discussion, &argument);
            }
        }
    }
} 


/**
 * @brief The main function of the server program.
 * 
 * @param argc The number of command-line arguments.
 * @param argv An array of strings containing the command-line arguments.
 * @return 0 on success, -1 on failure.
 */
int main(int argc, char *argv[]) {

    signal(SIGINT , shutdownserv);

    if (argc != 2) { //security : check if the number of arguments is correct
        perror("Incorrect number of arguments");
        printf("Usage : %s <port>\n", argv[0]);
        return -1;
    }
    printf("Start program\n");


    server_info info = setup_socket(argv[1]);

    int res = init_ressources(info.dS_Server);
    if (res < 0) {
        exit(-1);
    }

    res = client_init();

    int ecoute = listen(info.dS_Server,1);
    if (ecoute < 0) {
        perror("Connection error: listen failed\n");
        close(info.dS_Server);
        return ecoute;
    }
    //File handle
    file_socket = create_file_recup_socket(atoi(argv[1])); // Create a new socket to send files
    if (file_socket.socket == -1) {
        return -1; //error displayed by the function
    }

    printf("la valeur de file socket est : %s \n", file_socket.port);

    
    printf("Listening mode\n");

    pthread_t thread_add_client;

    int k = pthread_create(&thread_add_client, NULL, get_client, &info);

    //Waiting for the close of the thread

    pthread_join(thread_add_client,NULL);

    printf("Fin du programme");

    shutdownserv();

    return 0;
}