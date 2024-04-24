#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
//#include <sys/types.h>
#include <sys/sem.h>
#include "utilitaire.h"


//Command to launch this program : ./Serveur port
// Exemple : ./Serveur 3500


/**
 * @struct thread_argument
 * @brief Structure representing the arguments passed to a thread function.
 * 
 * This structure contains the file descriptor of the client connection and an array of client descriptors.
 */
struct thread_argument {
    int descripteur;        /**< The file descriptor of the client connection */
    int* tab_of_client;     /**< An array of client descriptors */
    int semaphore_id;
    int Nb_client_max;
};

/**
 * @struct arg_get_client
 * @brief Structure representing the arguments for getting clients.
 * 
 * This structure holds the necessary information for getting clients in the server.
 * It includes an array of client IDs, the maximum number of clients, and the server socket descriptor.
 */
struct arg_get_client {
    int* tab_client;        /**< Array of client IDs */
    int Nb_client_max;      /**< Maximum number of clients */
    int semaphore_id;
    int dS;               /**< Server socket descriptor */
};


/**
 * Creates a socket for TCP/IP communication.
 *
 * @return The socket file descriptor if successful, -1 otherwise.
 */
int creation_socket() {
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1) {
        perror("Connection error: socket creation failed\n");
        return dS;
    }
    printf("Socket created \n");
    return dS;
}

/**
 * Creates and returns a socket address structure based on the given port number.
 *
 * @param port The port number to be used for the socket address.
 * @return The created socket address structure.
 */
struct sockaddr_in param_socket_adresse(char *port) {
    // Get the addresses of the first Client
    struct sockaddr_in adresse1;
    adresse1.sin_family = AF_INET; // address family
    adresse1.sin_addr.s_addr = INADDR_ANY; // address to accept any incoming messages
    adresse1.sin_port = htons(atoi(port)); // port passed as argument
    printf("Adresse creation \n");
    return adresse1;
}

/**
 * Binds a socket to a specific address.
 *
 * @param socket The socket to bind.
 * @param adresse The address to bind the socket to.
 * @return Returns 0 on success, or an error code on failure.
 */
int make_bind(int socket, struct sockaddr_in adresse) {
    int connect = bind(socket, (struct sockaddr*)&adresse, sizeof(adresse)); //variable "connect" to avoid conflict with the function connect
    if (connect != 0) {
        perror("Connection error: bind failed\n");
        return connect;
    }
    printf("Socket named\n");
    return connect;
}

/**
 * Connects to a client.
 * 
 * This function accepts a client connection on the given socket descriptor.
 * It returns the new socket descriptor of the connected client.
 * 
 * @param adress The server address structure.
 * @param descripteur The socket descriptor of the server.
 * @return The new socket descriptor of the connected client, or -1 if an error occurred.
 */
int connect_to_client(struct sockaddr_in adress, int descripteur) {
    socklen_t lenght = sizeof(struct sockaddr_in); //keep the length of the address
    int dSClient = accept(descripteur, (struct sockaddr*) &adress, &lenght); //keep the (new) socket descriptor of the client
    if (dSClient == -1) {
        perror("Connection error: client failed to connect\n");
        return -1;
    }
    printf("\n-- Client connected --\n");
    return dSClient;
}

/**
 * Sends a message to all clients except the sender.
 * 
 * @param socket_sender The socket descriptor of the sender.
 * @param message The message to send.
 * @param tab_client The array of client socket descriptors.
 */
int send_all(int socket_sender, char *message, int *tab_client,int semaphore,int size) {
    semaphore_wait(semaphore);
    for (int i = 0; i<10; i++) {
        if (tab_client[i] != -1 && tab_client[i] != socket_sender) {
            int res = send_message(tab_client[i], message);
            if (res < 0) {
                perror("Error sending the message");
            }
        }
    }
    semaphore_unlock(semaphore);
    return 0;
}

int delete_client (int dS, int* tab_of_client,int semaphore) {
    int res = -1;
    int i = 0;
    semaphore_wait(semaphore);
    while (res == -1) {
        if (tab_of_client[i] == dS) {
            tab_of_client[i] = -1;
            res = 0;
        }
        i = i+1;
    }
    semaphore_unlock(semaphore);
    return res;
}

/**
 * Handles the conversation between two clients.
 * 
 * This function is responsible for managing the communication between two clients. It receives messages from one client and sends them to the other client.
 * 
 * @param arg The argument structure containing the socket descriptor and the array of client socket descriptors.
 */
void * discussion (void * arg) {
    struct thread_argument * argument = (struct thread_argument *) arg;
    short conversation = 1;
    char *message;
    int dS = argument->descripteur;
    while (conversation) {
        int res =  recv_message(dS, &message);
        printf("message recu : %s \n",message);
        if (res == 0) {
            puts("Deconnexion d'un client");
            int resultat = delete_client(dS,argument->tab_of_client,argument->semaphore_id);
            close(dS);
            pthread_exit(NULL);
        }
        else if (res < 0) {
            perror("Error receiving the message");
            exit(0);
        }
        else {
            res = send_all(dS, message, argument->tab_of_client,argument->semaphore_id,argument->Nb_client_max); 
        }
        sleep(0.01);
    }
}

/**
 * Adds a client to the client array.
 *
 * This function searches for an available slot in the client array and adds the client socket descriptor to it.
 *
 * @param tab_client The client array.
 * @param size The size of the client array.
 * @param dS The client socket descriptor to be added.
 * @return Returns 0 if the client was successfully added, -1 otherwise.
 */
int add_client(int *tab_client, int size, int dS,int semaphore) {
    int res = -1;
    int i = 0;
    semaphore_wait(semaphore);
    while (res == -1 && i < size) {
        if (tab_client[i] == -1) {
            tab_client[i] = dS;
            res = 0;
        }
        ++i;
    }
    semaphore_unlock(semaphore);
    return res;
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
void * get_client (void * arg ) {
    struct arg_get_client *args = (struct arg_get_client *) arg;
    while (1) {
        struct sockaddr_in aC ;
        int dSClient = connect_to_client(aC,args->dS);
        int res = add_client(args->tab_client,args->Nb_client_max,dSClient,args->semaphore_id);
        if (res == -1) {
            char message[] = "You can't connect there is already too many people connected, retry later";
            send_message(dSClient, message);
            close(dSClient);
        }
        else if (res == 0) {
            pthread_t tid;
            struct thread_argument argument = {dSClient,args->tab_client,args->semaphore_id,args->Nb_client_max};
            int i = pthread_create (&tid, NULL, discussion, &argument);
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

    if (argc != 2) { //security : check if the number of arguments is correct
        perror("Incorrect number of arguments");
        printf("Usage : %s <port>\n", argv[0]);
        return -1;
    }

    printf("Start program\n");
    //There is the const that define the maximum the number of client handled by the server
    const int NB_CLIENT_MAX = 10;

    short running = 1;    

    int dS = creation_socket();
    if (dS == -1) {
        return -1;
    }

    struct sockaddr_in adresse = param_socket_adresse(argv[1]);

    int connect = make_bind(dS,adresse);
    if (connect != 0) {
        close(dS);
        return -1;
    }

    int tab_client[NB_CLIENT_MAX];

    int cleSem = ftok("cle_sem.txt", 'r'); 

    int idSem = semget(cleSem, 1,0666);

    //Initialisation of all value of the tab
    int res = semaphore_wait(idSem);
    for (int i = 0; i<NB_CLIENT_MAX; ++i) {
        tab_client[i] = -1;
    }
    res = semaphore_unlock(idSem);

    int ecoute = listen(dS,1);
    if (ecoute < 0) {
        perror("Connection error: listen failed\n");
        close(dS);
        return ecoute;
    }
    
    printf("Listening mode\n");


    /*
    One of the problem is to find who is to know who is the sender and who is the receiver for the start, given that this is alternante next
    The idea here is that the both clients send their type to be affected the correct role
    */

    //Creating the thread that will handle the connection of new client

    pthread_t thread_add_client;

    struct arg_get_client arg_client = {tab_client,NB_CLIENT_MAX,idSem,dS};

    int k = pthread_create(&thread_add_client, NULL, get_client, &arg_client);

    //Waiting for the close of the thread

    pthread_join(thread_add_client,NULL);

    printf("Fin du programme");

    close(dS);

    return 0;
}