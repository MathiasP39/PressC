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

sem_t static semaphore_tableau;

sem_t static semaphore_nb_client;

struct client static *tab_client;


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
    struct client *tab_client;     /**< An array of client descriptors */
    sem_t semaphore_id;
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
    int socket;     ///< The socket associated with the client.
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
int send_all(int socket_sender, char *message) {
    sem_wait(&semaphore_tableau);
    for (int i = 0; i<NB_CLIENT_MAX; i++) {
        if (tab_client[i].socket != -1 && tab_client[i].socket != socket_sender) {
            int res = send_message(tab_client[i].socket, message);
            if (res < 0) {
                perror("Error sending the message");
            }
        }
    }
    sem_post(&semaphore_tableau);
    return 0;
}

int get_nickname(int dS, char **pseudo) {
    int i = 0;
    int res = -1;
    sem_wait(&semaphore_tableau);
    while (i<NB_CLIENT_MAX && res == -1) {
        if (tab_client[i].socket == dS) {
            strcpy(*pseudo,tab_client[i].nickname);
            strcat(*pseudo," : ");
            res = 0;
        }
        i++;
    }
    sem_post(&semaphore_tableau);
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
    int waitCheck = sem_wait(&semaphore_tableau); //wait for the semaphore to be available
    if (waitCheck == -1) {
        perror("sem_wait error : semop failed\n");
        return -1;
    }

    while (res == -1 && i< NB_CLIENT_MAX) {
        if (tab_client[i].socket == dS) {
            tab_client[i].socket = -1;
            tab_client[i].nickname = "";
            res = 0;
        }
        i = i+1;
    }

    int unlockCheck = sem_post(&semaphore_tableau); //unlock the semaphore previously locked
    if (unlockCheck == -1) {
        perror("sem_post error : semop failed\n");
        return -1;
    }
    send_message(dS,"Vous avez été déconnecté\n");
    int code = close(dS);
    if (code == 0) {
        puts("Fermeture de la socket réussite");
    }
    else if (code == -1) {
        perror("Error closing socket");
    }
    return res;
}

int get_dS(char * username) {
    int res = -1;
    int i = 0;
    sem_wait(&semaphore_tableau);
    while (i<NB_CLIENT_MAX && res == -1) {
        if (tab_client[i].socket != -1) {
            printf("Il y a l'utilisateur : %s qui a le dS : %d \n",tab_client[i].nickname,tab_client[i].socket);
        } 
        if ( strcmp(username,tab_client[i].nickname) == 0) {
            res = tab_client[i].socket;
        }
        i++;
    }
    sem_post(&semaphore_tableau);
    return res;
}


void whisper(char * username, char * message) {
    int req;
    int dS_cible = get_dS(username);
    if (dS_cible == -1) {
        perror("Utilisateur introuvable");
    }
    else {
        int res = send_message(dS_cible, message);
        if (res < 0) {
            perror("Error sending the message");
        }
    }
}

int kick(char * username) {
    int dS_cible = get_dS(username);
    delete_client(dS_cible);
}


int man(int descripteur) {
    puts("Passage dans la fonction man");
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

int shutdownserv(int dS) {
    sem_wait(&semaphore_tableau);
    for (int i = 0; i<NB_CLIENT_MAX; i++) {
        if (tab_client[i].socket != -1) {
            int res = send_message(tab_client[i].socket, "Le serveur va s'arrêter\n");
            if (res < 0) {
                perror("Error sending the message");
            }
        }
    }
    sem_post(&semaphore_tableau);
    close(dS);
    exit(0);
    return 1;
}

int list(int descripteur) {
    sem_wait(&semaphore_tableau);
    for (int i = 0; i<NB_CLIENT_MAX; i++) {
        if (tab_client[i].socket != -1) {
            char * message = tab_client[i].nickname;
            int res = send_message(descripteur, message);
        }
    }
    sem_post(&semaphore_tableau);
    return 1;
}

int quit (int descripteur) {
    int res = delete_client(descripteur);
    if (res == 0) {
        puts("Suppression réussie");
    }
    return 1;
}

/*
This function is in charge of detection of commands in a message
List of case value of return : 
    - 1 if nothing to do (/kick)
    - 2 if there is no command
    - -1 if there is a problem 
    - 0 if the user needs to be disconnected
*/
int analyse(char * arg, struct client *tab_client, sem_t semaphore, int descripteur) {
    if (arg[0] == '/') {
        char *tok = strtok(arg+1, " ");
        if (strcmp(tok, "kick") == 0) {
            tok = strtok(NULL, " ");
            kick(tok);
            return 1; // Nothing to do
        }else if (strcmp(tok, "whisper") == 0) {
            tok = strtok(NULL, " " );
            char * message = strtok(NULL, "\0");
            whisper(tok, message);
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
            return quit(descripteur);
        }else if (strcmp(tok, "shutdown") == 0) {
            shutdownserv(descripteur);
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

    //---The conversation loop---

    while (conversation) {
        int res =  recv_message(dS, &message);
        if (res <= 0) {
            if (res == 0) {
                perror("Error receiving the message");
                int resultat = delete_client(dS);
                if (resultat == 0) {
                puts("Suppression réussi");}
            }
            
            puts("Deconnexion d'un client");

            conversation = 0;
        }
        else {
            printf("Message recu : %s \n",message);
            char * pseudo = (char*) malloc(sizeof(char));// Variabe to store nickname
            int code = get_nickname(dS,&pseudo); //Here we get the nickname of the sender to add it to the message
            int rep = analyse(message, argument->tab_client, argument->semaphore_id, dS);
            if (rep == 2) { //Case no command
                strcat(pseudo,message);
                res = send_all(dS,pseudo);
            }else if (rep == 0) { //Case command /quit 
                shutdownserv(dS);
            }else if (rep == -1){
                puts("Error in the command");
                perror("Error");
            }
        }
    }
    puts("Sortie du thread");
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
int add_client(struct client *tab_client, int size, int dS, sem_t semaphore) {
    int res = -1;
    int i = 0;
    sem_wait(&semaphore_tableau);
    while (res == -1 && i < size) {
        if (tab_client[i].socket == -1) {
            tab_client[i].socket = dS;
            res = 0;
        }
        ++i;
    }
    sem_post(&semaphore_tableau);
    return res;
}



/**
 * Function to get the nickname of a client and store it in the client structure.
 * 
 * @param tab_client The array of client structures.
 * @param dS The socket descriptor of the client.
 * @param semaphore The semaphore used for synchronization.
 * @return 0 if successful, -1 otherwise.
 */
int set_nickname(struct client *tab_client, int Nb_client_max, int dS, sem_t semaphore) {
    int compt = -1;
    int i = 0;
    char *message = NULL;

    int waitCheck = sem_wait(&semaphore_tableau); //wait for the semaphore to be available
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
            for (int i = 0; i<Nb_client_max; i++) {
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
    for (int i = 0; i<Nb_client_max; i++) {
        if (tab_client[i].socket == dS) {
            tab_client[i].nickname = message;
        }
    }

    int check = send_message(dS, "\nBienvenue dans la discussion !\n\n");
    if (check == -1) {
        perror("Error sending the message");
        return -1;
    }

    int unlockCheck = sem_post(&semaphore_tableau); //unlock
    if (unlockCheck == -1) {
        perror("semaphore_unlock error");
        return -1;
    }

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
    while (1) {
        struct sockaddr_in aC ;
        int dSClient = connect_to_client(aC,args->dS);
        int res = sem_trywait(&semaphore_nb_client);
        if (res == -1) {
            char message[] = "You can't connect there is already too many people connected, retry later";
            send_message(dSClient, message);
            close(dSClient);
        }
        else if (res == 0) {
            puts("Debut ajout client");
            res = add_client(tab_client, NB_CLIENT_MAX, dSClient, semaphore_tableau);
            puts("Client ajouté");
            res = set_nickname(tab_client, NB_CLIENT_MAX, dSClient, semaphore_tableau);
            if (res == -1) {
                perror("Error getting the nickname");
                close(dSClient);
            }
            pthread_t tid;
            struct thread_argument argument = {dSClient,tab_client,semaphore_tableau,NB_CLIENT_MAX};
            int i = pthread_create (&tid, NULL, discussion, &argument);
        }
    }
} 

void extinction () {
    puts("Le serveur va s'éteindre ...");
    sleep(1);
    exit(0);
}

/**
 * @brief The main function of the server program.
 * 
 * @param argc The number of command-line arguments.
 * @param argv An array of strings containing the command-line arguments.
 * @return 0 on success, -1 on failure.
 */
int main(int argc, char *argv[]) {

    signal(SIGINT ,extinction);

    if (argc != 2) { //security : check if the number of arguments is correct
        perror("Incorrect number of arguments");
        printf("Usage : %s <port>\n", argv[0]);
        return -1;
    }
    printf("Start program\n");
    //There is the const that define the maximum the number of client handled by the server

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

    tab_client = (struct client*)malloc(NB_CLIENT_MAX * sizeof(struct client));

    sem_t sem_nb_client;

    int res = sem_init(&semaphore_nb_client,0,NB_CLIENT_MAX);

    sem_t sem;

    res = sem_init(&semaphore_tableau,0,1);

    //Initialisation of all value of the tab
    res = sem_wait(&sem);
    for (int i = 0; i<NB_CLIENT_MAX; ++i) {
        tab_client[i].nickname = "";
        tab_client[i].socket = -1;
    }
    res = sem_post(&sem);
    //Gestion erreur

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

    int k = pthread_create(&thread_add_client, NULL, get_client, &dS);

    //Waiting for the close of the thread

    pthread_join(thread_add_client,NULL);

    printf("Fin du programme");

    close(dS);

    return 0;

}