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

int static server_running = 1;

int static Serveur_dS;

struct socket_info file_socket;


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
    pthread_t thread_identifier;
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
    pthread_t thread_identifier;
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
    // printf("Adresse creation \n");
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
    int *val = (int*)malloc(sizeof(int));
    sem_getvalue(&semaphore_tableau,val);
    puts("Semaphore nickname");
    int error = sem_wait(&semaphore_tableau);
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
    puts("Semaphore delete client");
    int waitCheck = sem_wait(&semaphore_tableau); //wait for the semaphore to be available
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

    int unlockCheck = sem_post(&semaphore_tableau); //unlock the semaphore previously locked
    if (unlockCheck == -1) {
        perror("sem_post error : semop failed\n");
        return -1;
    }
    puts("Relache semaphore delete client");
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
    puts("Semaphore getDs");
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
 * @return 1 on success, -1 on failure.
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
    return 1;
}
/**
 * Function to display the contents of a file to a client.
 * 
 * @param descripteur The descriptor of the client.
 * @return 1 on success, -1 on failure.
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
    return 1;
}

/**
 * Shuts down the server and sends a message to all connected clients.
 * 
 * @param dS The server socket descriptor.
 * @param tab_client An array of client structures.
 * @param semaphore The semaphore used for synchronization.
 * @return 1 if the shutdown is successful, -1 otherwise.
 */
int shutdownserv(int dS) {
    puts("Semaphore shutdown");
    sem_wait(&semaphore_tableau);
    for (int i = 0; i<NB_CLIENT_MAX; i++) {
        if (tab_client[i].socket != -1) {
            int res = send_message(tab_client[i].socket, "Le serveur va s'arrêter\n");
            if (res < 0) {
                puts("Error sending the message");
                return -1;
            }
        }
    }
    sem_post(&semaphore_tableau);
    close(dS);
    return 1;
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
    puts("Semaphore List");
    sem_wait(&semaphore_tableau);
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
    sem_post(&semaphore_tableau);
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


// FILE SPECIAL COMMAND HANDLE


/**
 * Lists the files in the server's 'biblio' library and send the list to the client.
 * Uses update_file_list(const char* directory) function from utilitaire.c, which returns a string containing the list of files.
 * 
 * @param descripteur The client socket descriptor.
 * @return 1 if the list is successfully sent, -1 otherwise.
 */
int filelist(int descripteur) {
    char* file_list = update_file_list("./biblio");
    if (file_list == NULL) {
        perror("Unable to get file list");
        return -1;
    }

    ssize_t bytes_sent = send(descripteur, file_list, strlen(file_list), 0);
    free(file_list);  // Free the memory allocated by update_file_list

    if (bytes_sent == -1) {
        perror("Failed to send file list");
        return -1;
    }

    return 1;
}

/**
 * Checks if a file exists.
 * 
 * @param filename The name of the file to check.
 * @return 1 if the file exists, -1 otherwise.
 */
int check_file_exists(const char* filename) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "biblio/%s", filename); // Check in the 'biblio' directory

    FILE* file = fopen(filepath, "r");
    if (file == NULL) {
        perror("recup_file: error opening the file");
        return -1;
    }
    fclose(file);
    printf("File %s exists\n", filename);
    return 1;
}

/**
 * Finds a free port on the server.
 * 
 * @return The first free port found, or -1 if no free port is found.
 */
int get_free_port(int server_port) {
    int port = server_port + 1; // Start from the server port + 1
    char port_str[6]; // Buffer to hold the port number as a string

    while (port < 65535) {
        int test_socket = socket(PF_INET, SOCK_STREAM, 0);
        sprintf(port_str, "%d", port); // Convert the port number to a string
        struct sockaddr_in adresse = param_socket_adresse(port_str);
        if (bind(test_socket, (struct sockaddr*)&adresse, sizeof(adresse)) == 0) {
            close(test_socket);
            printf("Found free port: %d\n", port);
            return port;
        }
        close(test_socket);
        port++;
    }
    return -1; // No free port found
}

/**
 * Structure representing a socket and its address.
 */
struct socket_info {
    int socket;
    struct sockaddr_in adresse;
    char* port;
};

/**
 * Creates a socket for file recovery.
 * 
 * @param server_port The port number of the server.
 * @return The socket information structure.
 */
struct socket_info create_file_recup_socket(int server_port) {
    struct socket_info info;
    info.socket = socket(PF_INET, SOCK_STREAM, 0);

    int port_num = get_free_port(server_port);
    char* port = malloc(6 * sizeof(char)); // Maximum 5 digits for a port number + null terminator
    if (port == NULL) {
        perror("Failed to allocate memory for port");
        return info;
    }

    sprintf(port, "%d", port_num);
    if (port == "-1") {
        close(info.socket);
        info.socket = -1;
        return info;
    }
    info.port = port;

    info.adresse = param_socket_adresse(port);
    if (make_bind(info.socket, info.adresse) != 0) {
        close(info.socket);
        info.socket = -1;
        return info;
    }

    if (listen(info.socket, 1) < 0) {
        close(info.socket);
        info.socket = -1;
        return info;
    }
    printf("File recovery socket created on port %s\n", port);

    return info;
}


/**
 * Sends a file to a client.
 * 
 * @param info The socket information structure.
 * @param filename The name of the file to send.
 * @return 1 if the file is successfully sent, -1 otherwise.
*/
void* file_recup_socket(void* arg) {
    char* filename = (char*)arg;
    struct socket_info info = file_socket; // Get the file sending socket information

    if (info.socket == -1) {
        return (void*)-1;
    }

    int dSClient = connect_to_client(info.adresse, info.socket);
    if (dSClient == -1) {
        return (void*)-1;
    }

    char filepath[256];
    sprintf(filepath, "./biblio/%s", filename);

    if (send_file(dSClient, filepath) == -1) {
        return (void*)-1;
    }
    printf("File %s sent\n", filename);

    // After sending the file
    if (shutdown(dSClient, SHUT_RDWR) == -1) {
        perror("Failed to shutdown the socket");
        return (void*)-1;
    }
    printf("Client disconnected from file recup\n");

    return (void*)1;
}

/**
 * Sends a file to a client using a new thread.
 * Uses the recup_file function to send the file.
 * 
 * @param dS The client socket descriptor.
 * @param filename The name of the file to send.
 * @return 1 if the thread is successfully created, -1 otherwise.
*/
int file_recup_thread(int dS, char * filename) {
    int fileExists = check_file_exists(filename);
    if (fileExists == -1) {
        perror("Error : unreachable file");
        return -1;
    }

    pthread_t tid;
    struct thread_argument argument = {dS, tid};
    int i = pthread_create(&tid, NULL, file_recup_socket, filename);
    if (i != 0) {
        perror("Error creating the thread");
        return -1;
    }

    // Sends a message to the client to indicate that the file is being sent
    char notification[256];
    sprintf(notification, "/receiving %s on %s", filename, file_socket.port);
    int res = send_message(dS, notification);
    if (res < 0) {
        perror("Error sending the notification");
        return -1;
    }

    // Wait for the thread to finish
    void* result;
    if (pthread_join(tid, &result) != 0) {
        perror("Error joining the thread");
        return -1;
    }

    // Check the result of file_recup_socket
    if (result == NULL) {
        perror("Error in file_recup_socket");
        return -1;
    }
    printf("Thread created for file %s\n", filename);

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
int analyse(char * arg, int descripteur) {
    puts("Passage dans analyse");
    puts("Reperage du /");
    if (arg[0] == '/') {
        puts("Reperage du /");
        char *tok = strtok(arg+1, " ");
        if (strcmp(tok, "kick") == 0) {
            tok = strtok(NULL, " ");
            return kick(tok);
        }else if (strcmp(tok, "whisper") == 0) {
            tok = strtok(NULL, " " );
            char * message = strtok(NULL, "\0");
            return whisper(descripteur,tok, message);
        }else if (strcmp(tok, "close") == 0) {
            return 0; //  0 = need to deconnect
        }else if (strcmp(tok, "man") == 0) {
            return man(descripteur);
        }else if (strcmp(tok, "list") == 0) {
            return list(descripteur);
        }else if (strcmp(tok, "quit") == 0) {
            puts("Passage dans quit");
            return quit(descripteur);
        }else if (strcmp(tok, "shutdown") == 0) {
            return shutdownserv(descripteur);
        }else if (strcmp(tok, "biblio") == 0) {
            return filelist(descripteur);
        }else if (strcmp(tok, "recup") == 0) {
            tok = strtok(NULL, " ");
            return file_recup_thread(descripteur, tok);
        }
        puts("Aucune commande correspondante");
    }else {return 2;}
}


int set_thread_id (int dS, pthread_t thread_id) {
    int res = -1;
    int i = 0 ;
    puts("Semaphore set thread id");
    int code = sem_wait(&semaphore_tableau);

    while (res == -1 && i<NB_CLIENT_MAX) {
        if (tab_client[i].socket == dS) {
            tab_client[i].thread_identifier = thread_id;
            res = 0;
        }
        i++;
    }

    code = sem_post(&semaphore_tableau);
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
    int conversation = 1;
    char *message = NULL; //The message received. Initialized to NULL to avoid recv_message to free a non-allocated memory
    int dS = argument->descripteur;

    char * sender = (char*) malloc(sizeof(char));
    int code = get_nickname( dS, &sender);
    if (code == -1) {
        perror("Error getting the nickname");
    }
    sender[strcspn(sender, " :")] = '\0'; //Remove the " :" from the nickname
    int res = set_thread_id(dS,argument->thread_identifier);
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
            puts("avant allocation mémoire");
            char * pseudo = (char*) malloc(sizeof(char));// Variabe to store nickname
            puts("avant getnickname");
            int code = get_nickname(dS,&pseudo); //Here we get the nickname of the sender to add it to the message
            puts("apres get nickname");
            int rep = analyse(message, dS);
            if (rep == 2) { //Case no command
                strcat(pseudo,message);
                res = send_all(dS,pseudo);
                free(pseudo);
            }else if (rep == 0) { //Case command /quit 
                shutdownserv(dS);
            }else if (rep == -1){
                puts("Error in the command");
            }
        }
    }
    free(message);
    pthread_kill(pthread_self(), SIGUSR1);
    puts("envoie du signal réussi");
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
    puts("Semaphore add client");
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

void clean_client () {
    puts("passage dans clean client");
    int res = -1;
    int i = 0;
    puts("Semaphore clean client");
    int waitCheck = sem_wait(&semaphore_tableau); //wait for the semaphore to be available
    if (waitCheck == -1) {
        perror("sem_wait error : semop failed\n");
    }
    else {
        while ( i< NB_CLIENT_MAX && res == -1) {
            puts("passage dans la boucle");
            if (tab_client[i].thread_identifier != -1) {
                if (pthread_join(tab_client[i].thread_identifier,NULL) == 0) {
                    puts("suppression d'un utilisateur");
                    tab_client[i].socket = -1;
                    tab_client[i].nickname = "";
                    tab_client[i].thread_identifier = -1;
                    res = 0;
                }
            }
            i = i+1;
        }
    }
    int waitFree = sem_post(&semaphore_tableau);
    puts("fin clean client");
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
    puts("Semaphore set nickname");
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
            puts("Debut ajout client");
            res = add_client(tab_client, NB_CLIENT_MAX, dSClient, semaphore_tableau);
            puts("Client ajouté");
            res = set_nickname(dSClient);
            if (res == -1) {
                perror("set : Error getting the nickname");
                close(dSClient);
            }
            pthread_t tid;
            struct thread_argument argument = {dSClient, tid};
            int i = pthread_create (&tid, NULL, discussion, &argument);
        }
    }
} 

int clean_all_threads () {
    int res = 0;
    puts("Semaphore clean all");
    int code = sem_wait(&semaphore_tableau);

    int i = 0;

    while (i < NB_CLIENT_MAX) {
        if (tab_client[i].socket != -1) {
            pthread_join(tab_client[i].thread_identifier,NULL);
            tab_client[i].thread_identifier = -1;
        }
        i++;
    }

    code = sem_post(&semaphore_tableau);
    return res;
}

void extinction () {
    puts("\nLe serveur va s'éteindre ...");
    server_running = 0;
    clean_all_threads();
    shutdown(Serveur_dS,SHUT_RDWR );
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

    //signal(SIGINT ,extinction);

    signal(SIGUSR1, clean_client);

    if (argc != 2) { //security : check if the number of arguments is correct
        perror("Incorrect number of arguments");
        printf("Usage : %s <port>\n", argv[0]);
        return -1;
    }
    printf("Start program\n");
    //There is the const that define the maximum the number of client handled by the server

    int Serveur_dS = creation_socket();
    if (Serveur_dS == -1) {
        return -1;
    }

    struct sockaddr_in adresse = param_socket_adresse(argv[1]);

    int connect = make_bind(Serveur_dS,adresse);
    if (connect != 0) {
        close(Serveur_dS);
        return -1;
    }

    //File handle
    file_socket = create_file_recup_socket(Serveur_dS); // Create a new socket to send files
    if (file_socket.socket == -1) {
        return -1; //error displayed by the function
    }
    //File handle

    tab_client = (struct client*)malloc(NB_CLIENT_MAX * sizeof(struct client));


    int res = sem_init(&semaphore_nb_client,0,NB_CLIENT_MAX);


    res = sem_init(&semaphore_tableau,0,1);

    //Initialisation of all value of the tab
    res = sem_wait(&semaphore_tableau);
    for (int i = 0; i<NB_CLIENT_MAX; ++i) {
        tab_client[i].nickname = "";
        tab_client[i].socket = -1;
        tab_client[i].thread_identifier = -1;
    }
    res = sem_post(&semaphore_tableau);
    //Gestion erreur

    int ecoute = listen(Serveur_dS,1);
    if (ecoute < 0) {
        perror("Connection error: listen failed\n");
        close(Serveur_dS);
        return ecoute;
    }
    
    printf("Listening mode\n");


    /*
    One of the problem is to find who is to know who is the sender and who is the receiver for the start, given that this is alternante next
    The idea here is that the both clients send their type to be affected the correct role
    */

    //Creating the thread that will handle the connection of new client

    pthread_t thread_add_client;

    int k = pthread_create(&thread_add_client, NULL, get_client, &Serveur_dS);

    //Waiting for the close of the thread

    pthread_join(thread_add_client,NULL);

    printf("Fin du programme");

    extinction();

    return 0;

}