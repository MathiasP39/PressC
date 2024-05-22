#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
#include "utilitaire.h"
#include <signal.h>
#define NB_CLIENT_MAX 10

sem_t static semaphore_nb_client;

pthread_mutex_t mutex_tab_cli;

struct client static *tab_client;

int static server_running = 1;

int static Serveur_dS ;


//Command to launch this program : ./Serveur port
// Exemple : ./Serveur 3500

//-----STRUCTURES-----


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
 * @struct client
 * @brief Represents a client connected to the server.
 * 
 * This struct contains information about a client, including their nickname and socket.
 */
struct client {
    char *nickname; ///< The nickname of the client.
    int socket;
};


//-----FUNCTIONS-----


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
        perror("Connection error: bind failed");
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
int send_all(int socket_sender, char *message) {
    pthread_mutex_lock(&mutex_tab_cli);
    for (int i = 0; i<NB_CLIENT_MAX; i++) {
        if (tab_client[i].socket != -1 && tab_client[i].socket != socket_sender) {
            int res = send_message(tab_client[i].socket, message);
            if (res < 0) {
                perror("Error sending the message");
            }
        }
    }
    pthread_mutex_unlock(&mutex_tab_cli);
    return 0;
}

/**
 * Retrieves the nickname of a client based on their socket descriptor.
 *
 * @param tab_client The array of client structures.
 * @param dS The socket descriptor of the client.
 * @param pseudo A pointer to a string to store the client's nickname.
 * @param nb_client_max The maximum number of clients.
 * @param semaphore The semaphore used for synchronization.
 * @return 0 if the nickname was successfully retrieved, -1 otherwise.
 */
int get_nickname(int dS, char **pseudo) {
    int i = 0;
    int res = -1;
    int error = pthread_mutex_lock(&mutex_tab_cli);
    if (error < 0) {
        perror("Error while trying to access semaphore");
    }
    while (i<NB_CLIENT_MAX && res == -1) {
        if (tab_client[i].socket == dS) {
            strcpy(*pseudo, tab_client[i].nickname);
            strcat(*pseudo," : ");
            res = 0;
        }
        i++;
    }
    pthread_mutex_unlock(&mutex_tab_cli);
    return res;
}

/**
 * Deletes a client from the client array.
 * 
 * This function searches for the client socket descriptor in the client array and deletes it. It deletes by setting the value of the client socket descriptor to -1.
 * 
 * @param dS The client socket descriptor to delete.
 * @param tab_client The array of client socket descriptors.
 * @return Returns 0 if the client was successfully deleted, -1 otherwise.
 */
int delete_client (int dS) {
    int res = -1;
    int i = 0;
    int waitCheck = pthread_mutex_lock(&mutex_tab_cli);
    if (waitCheck == -1) {
        perror("sem_wait error : semop failed\n");
        return -1;
    }
    while (res == -1 && i< NB_CLIENT_MAX) {
        if (tab_client[i].socket == dS) {
            printf("le nom du supprimé est : %s et sa socket vaut %d\n",tab_client[i].nickname,tab_client[i].socket);
            tab_client[i].socket = -1;
            tab_client[i].nickname = "";
            res = 0;
        }
        i = i+1;
    }

    int unlockCheck = pthread_mutex_unlock(&mutex_tab_cli);
    if (unlockCheck == -1) {
        perror("sem_post error : semop failed\n");
        return -1;
    }
    return res;
}


/**
 * Retrieves the socket descriptor (dS) associated with a given username from an array of clients.
 *
 * @param username The username to search for.
 * @param tab_client The array of clients.
 * @param semaphore The semaphore used for synchronization.
 * @return The socket descriptor (dS) associated with the username, or -1 if not found.
 */
int get_dS(char * username) {
    int res = -1;
    int i = 0;
    pthread_mutex_lock(&mutex_tab_cli);
    while (i<NB_CLIENT_MAX && res == -1) {
        if (tab_client[i].socket != -1) {
            printf("Il y a l'utilisateur : %s qui a le dS : %d \n",tab_client[i].nickname,tab_client[i].socket);
        } 
        if ( strcmp(username,tab_client[i].nickname) == 0) {
            res = tab_client[i].socket;
        }
        i++;
    }
    pthread_mutex_unlock(&mutex_tab_cli);
    return res;
}

/**
 * Sends a private message to a specific user.
 *
 * @param username The username of the recipient.
 * @param message The message to be sent.
 * @param tab_client An array of client structures.
 * @param semaphore The semaphore used for synchronization.
 * @return Returns 1 on success, -1 on failure.
 */
int whisper(int sender_dS,char * username, char * message) {
    int req;
    int dS_cible = get_dS(username);
    if (dS_cible == -1) {
        printf("Unknown user\n");
        return -1;
    }

    char * sender = (char*) malloc(sizeof(char));
    int code = get_nickname(sender_dS, &sender);
    if (code == -1) {
        printf("Error getting the nickname\n");
        return -1;
    }
    sender[strcspn(sender, " :")] = '\0'; //Remove the " :" from the nickname

    char * coloredmsg = (char*) malloc(strlen(message) + strlen(sender) + 32 + 5); //32 is the length of the string "{sender} vous chuchotte : " + colors and 5 is for the null terminator + possible mistakes
    if (coloredmsg == NULL) {
        printf("Error allocating memory\n");
        return -1;
    }

    sprintf(coloredmsg,"\033[1;31m%s vous chuchotte : %s\033[0m", sender, message);//Color the message in red
    printf("Message privé de %s à %s est : \"%s\" \n", sender, username, message);

    int res = send_message(dS_cible, coloredmsg);
    if (res < 0) {
        printf("Error sending the message\n");
    }

    free(coloredmsg);
    free(sender);
    return res;
}

/**
 * Function to kick a client from the server.
 * 
 * @param username The username of the client to be kicked.
 * @param tab_client The array of client structures.
 * @param semaphore The semaphore used for synchronization.
 * @return 0 on success, -1 on failure.
 */
int kick(char * username) {
    int dS_cible = get_dS(username);
    if (dS_cible == -1) {
        printf("Unknown user\n");
        return -1;
    }
    int deleteCheck = delete_client(dS_cible);
    if (deleteCheck != 0) {
        printf("Error kicking the client\n");
    }
    close(dS_cible);
    return deleteCheck;
}
/**
 * Function to display the contents of a file to a client.
 * 
 * @param descripteur The descriptor of the client.
 * @return 0 on success, -1 on failure.
 */
int man(int descripteur) {
    FILE* fichier = NULL;
    char chaine[100] = "";
    fichier = fopen("commande.txt", "r");
    if (fichier != NULL) {
        while (fgets(chaine, 100, fichier) != NULL) {
            char *message = chaine;
            send_message(descripteur, message);
        }
        fclose(fichier);
    }
    return 0;
}

/**
 * Shuts down the server and sends a message to all connected clients.
 * 
 * @param dS The server socket descriptor.
 * @param tab_client An array of client structures.
 * @param semaphore The semaphore used for synchronization.
 * @return 1 if the shutdown is successful, -1 otherwise.
 */
void shutdownserv() {
    puts("\nLe serveur va s'éteindre ...");
    send_all(-1, "Le serveur va s'arreter, vous allez etre deconnecter \n");
    server_running = 0;
    shutdown(Serveur_dS,SHUT_RDWR );
    sleep(1);
    exit(0);
}

/**
 * Sends a list of connected clients to a specific client.
 * 
 * @param tab_client An array of client structures.
 * @param semaphore The semaphore used for synchronization.
 * @param descripteur The client socket descriptor.
 * @return 1 if the list is successfully sent, -1 otherwise.
 */

int list(int descripteur) {
    pthread_mutex_lock(&mutex_tab_cli);
    for (int i = 0; i<NB_CLIENT_MAX; i++) {
        if (tab_client[i].socket != -1) {
            char * message = tab_client[i].nickname;
            int res = send_message(descripteur, message);
            if (res < 0) {
                puts("Error sending the message");
                return -1;
            }
        }
    }
    pthread_mutex_unlock(&mutex_tab_cli);
    return 1;
}

/**
 * Removes a client from the server.
 * 
 * @param descripteur The client socket descriptor.
 * @param tab_client An array of client structures.
 * @param semaphore The semaphore used for synchronization.
 * @return 1 if the client is successfully removed, -1 otherwise.
 */
int quit (int descripteur) {
    int res = delete_client(descripteur);
    if (res == 0) {
        close(descripteur);
        puts("Suppression réussie");
        return 1;
    }
    else {
        puts("Dans quit : suppression raté");
        return -1;
    }
}

/*
This function is in charge of detection of commands in a message
List of case value of return : 
    - 1 if nothing to do (/kick)
    - 2 if there is no command
    - -1 if there is a problem 
    - 0 if the user needs to be disconnected
*/
int analyse(char * arg, int descripteur) {
    if (arg[0] == '/') {
        char *tok = strtok(arg+1, " ");
        if (strcmp(tok, "kick") == 0) {
            tok = strtok(NULL, " ");
            kick(tok);
            return 1; // Nothing to do
        }else if (strcmp(tok, "whisper") == 0) {
            tok = strtok(NULL, " " );
            char * message = strtok(NULL, "\0");
            whisper(descripteur,tok, message);
            return 1;
        }else if (strcmp(tok, "close") == 0) {
            return 0; //  0 = need to deconnect
        }else if (strcmp(tok, "man") == 0) {
            man(descripteur);
            return 1;
        }else if (strcmp(tok, "list") == 0) {
            list(descripteur);
            return 1;
        }else if (strcmp(tok, "quit") == 0) {
            puts("passage dans quit");
            return quit(descripteur);
        }else if (strcmp(tok, "shutdown") == 0) {
            shutdownserv();
        }
    }else {return 2;}
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
    int conversation = 1;
    char *message = NULL; //The message received. Initialized to NULL to avoid recv_message to free a non-allocated memory
    int dS = argument->descripteur;

    char * sender = (char*) malloc(sizeof(char));
    int code = get_nickname( dS, &sender);
    if (code == -1) {
        perror("Error getting the nickname");
    }
    sender[strcspn(sender, " :")] = '\0'; //Remove the " :" from the nickname

    if (res == -1) {
        perror("Clients doesn't exist");
    }

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
 * Adds a client to the client array.
 *
 * This function searches for an available slot in the client array and adds the client socket descriptor to it.
 *
 * @param tab_client The client array.
 * @param size The size of the client array.
 * @param dS The client socket descriptor to be added.
 * @return Returns 0 if the client was successfully added, -1 otherwise.
 */
int add_client(int dS) {
    int res = -1;
    int i = 0;
    pthread_mutex_lock(&mutex_tab_cli);
    while (res == -1 && i < NB_CLIENT_MAX) {
        if (tab_client[i].socket == -1) {
            tab_client[i].socket = dS;
            res = 0;
        }
        ++i;
    }
    pthread_mutex_unlock(&mutex_tab_cli);
    return res;
}

void clean_client () {
    int res = -1;
    int i = 0;
    int waitCheck = pthread_mutex_lock(&mutex_tab_cli);
    if (waitCheck == -1) {
        perror("sem_wait error : semop failed\n");
    }
    else {
        while ( i< NB_CLIENT_MAX && res == -1) {
            if (tab_client[i].thread_identifier != -1) {
                tab_client[i].socket = -1;
                tab_client[i].nickname = "";
                res = 0;
            }
            i = i+1;
        }
    }
    pthread_mutex_unlock(&mutex_tab_cli);
}

/**
 * Function to get the nickname of a client and store it in the client structure.
 * 
 * @param tab_client The array of client structures.
 * @param dS The socket descriptor of the client.
 * @param semaphore The semaphore used for synchronization.
 * @return 0 if successful, -1 otherwise.
 */
int set_nickname(int dS) {
    int compt = -1;
    int i = 0;
    char *message = NULL;
    int waitCheck = pthread_mutex_lock(&mutex_tab_cli);
    if (waitCheck == -1) {
        perror("sem_wait error");
        return compt;
    }

    while (compt == -1) {
        compt = 0;
        int res = recv_message(dS, &message);
        if (res == 0) {
            puts("Annulation de connexion d'un client");
            int resultat = delete_client(dS);
            close(dS);
            pthread_exit(NULL);
        }
        else if (res < 0) {
            perror("Error receiving the nickname");
            exit(0);
        }
        else {
            printf("Pseudo recu : %s \n",message);
            for (int i = 0; i<NB_CLIENT_MAX; i++) {
                if (strcmp(tab_client[i].nickname,message) == 0) {
                    int sendCheck = send_message(dS, "Pseudo indisponible, veuillez en choisir un autre :\n");
                    if (sendCheck == -1) {
                        perror("Error sending the message");
                        exit(0);
                    }
                    compt = -1;
                }
            }
        }
    }
    for (int i = 0; i<NB_CLIENT_MAX; i++) {
        if (tab_client[i].socket == dS) {
            tab_client[i].nickname = message;
        }
    }

    int check = send_message(dS, "\nBienvenue dans la discussion !\n\n");
    if (check == -1) {
        perror("Error sending the message");
        return -1;
    }

    pthread_mutex_unlock(&mutex_tab_cli);

    return compt;
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
    struct arg_get_client *args = (struct arg_get_client *) arg;
    while (server_running) {
        struct sockaddr_in aC ;
        int dSClient = connect_to_client(aC,args->dS);
        int res = sem_trywait(&semaphore_nb_client);
        if (res == -1) {
            char message[] = "You can't connect there is already too many people connected, retry later";
            send_message(dSClient, message);
            close(dSClient);
        }
        else if (res == 0) {
            res = add_client(dSClient);
            res = set_nickname(dSClient);
             puts("On est la3");
            if (res == -1) {
                perror("set : Error getting the nickname");
                close(dSClient);
            }
            puts("Client ajouté");  
            pthread_t tid;
            struct thread_argument argument = {dSClient, tid};
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

    signal(SIGINT , shutdownserv);

    //Gestion erreur

    /*
    One of the problem is to find who is to know who is the sender and who is the receiver for the start, given that this is alternante next
    The idea here is that the both clients send their type to be affected the correct role
    */


}