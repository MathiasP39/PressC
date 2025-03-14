#include <stdio.h>
#include<unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "ressources.h"
#include "../lib_headers/utils.h"
#include "ServeurConnection.h"
#define NB_CHANEL 10 //Number of chanel should be of 1 or more

static pthread_mutex_t mutex_tab_cli;

static pthread_mutex_t mutex_tab_chanel;

/**
 * @struct thread_argument
 * @brief Structure representing the arguments passed to a thread function.
 * 
 * This structure contains the file descriptor of the client connection and an array of client descriptors.
 */
struct thread_argument {
    int descripteur;        /**< The file descriptor of the client connection */
};
struct file_reception_args {
    int descripteur;
    char *filename;
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

struct chanel {
    char *name;
    int* list_of_client;
};

struct client static *tab_client;
struct chanel static *tab_chanel;

/**
 * Function that inits all the value of this field, like a constructor in object oriented programming
*/
int client_init () {
    tab_client = (struct client*)malloc(NB_CLIENT_MAX * sizeof(struct client));
    tab_chanel = (struct chanel*)malloc(NB_CHANEL * sizeof(struct chanel));

    int res = pthread_mutex_init(&mutex_tab_cli,NULL);
    if (res < 0) {
        perror("Init of mutex, error while initialising");
        exit(-1);
    }

    res = pthread_mutex_init(&mutex_tab_chanel,NULL);
    if (res < 0) {
        perror("Init of mutex, error while initialising");
        exit(-1);
    }

    //Initialisation of all value of the tab of clients
    pthread_mutex_lock(&mutex_tab_cli);
    for (int i = 0; i<NB_CLIENT_MAX; ++i) {
        tab_client[i].nickname = "";
        tab_client[i].socket = -1;
    }
    pthread_mutex_unlock(&mutex_tab_cli);

    //So we initialise the table of chanel, this table contains the name and the list of client connected to it.
    pthread_mutex_lock(&mutex_tab_chanel);
    tab_chanel[0].list_of_client = (int*)malloc(NB_CLIENT_MAX*sizeof(int));
    tab_chanel[0].name = "General";
    for (int i = 0; i<NB_CLIENT_MAX;++i) {
        tab_chanel[0].list_of_client[i]=-1;
    }
    for (int i = 1; i<NB_CHANEL;++i) {
        tab_chanel[i].name = "";
        tab_chanel[i].list_of_client = (int*)malloc(NB_CLIENT_MAX*sizeof(int));
        for (int j = 0; j<NB_CLIENT_MAX;++j) {
            tab_chanel[i].list_of_client[j]=-1;
        }
    }
    pthread_mutex_unlock(&mutex_tab_chanel);
    return 0;
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
 * This function is in charge to send the message in the corresponding chanel of the Client
 * 
 * @param socket_sender socket descriptor from the client that is the sender
 * @param message the message to send through the chanel 
 * @return 1 if all went good
 */
int send_chanel(int socket_sender, char *message) {
    pthread_mutex_lock(&mutex_tab_chanel);
    int res = 0;
    int j = 0;
    while (res == 0 && j<NB_CHANEL) {
        int i = 0;
        short trouve = 0;
        while (i<NB_CLIENT_MAX && trouve == 0) {
            if (tab_chanel[j].list_of_client[i] == socket_sender) {
                trouve = 1;
            }
            i++;
        }
        if (trouve == 1) {
            i = 0;
            while (i<NB_CLIENT_MAX){
                if (tab_chanel[j].list_of_client[i] != -1 && tab_chanel[j].list_of_client[i] != socket_sender){
                    send_message(tab_chanel[j].list_of_client[i],message);
                }
                i++;
            }
        }
        j++;
    }
    pthread_mutex_unlock(&mutex_tab_chanel);
    return 1;
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
        if ( strcmp(username,tab_client[i].nickname) == 0) {
            res = tab_client[i].socket;
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
 * This function handle the mechanism of file reception from the client 
 * 
 * @param descripteur The socket descriptor of the client that is gonna send the file 
 * @param args The name of the file that will be send and created server side
*/
void* file_reception(int descripteur, char* args) {
    char message[256];
    snprintf(message, sizeof(message), "/send %s %s", args, file_socket.port);
    send_message(descripteur, message); //We send the port of the socket where the file will be send 
    char buffer[1024];
    char file_name[256];
    snprintf(file_name, sizeof(file_name), "./biblio/%s", args); //We create the filepath for the file handling
    printf("nom du fichier %s\n", file_name);
    int file_fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666); //We create the file 
    if (file_fd == -1) {
        perror("Failed to open file");
        return NULL;
    }
    printf("File opened\n");

    ssize_t bytes_received;
    //Loop that is in charge of receiving the file, unless if it is shrinked
    while ((bytes_received = recv(descripteur, buffer, sizeof(buffer), 0)) > 0) { 
        if (write(file_fd, buffer, bytes_received) == -1) {
            perror("Failed to write to file");
            close(file_fd);
            return NULL;
        }
    }

    if (bytes_received == -1) {
        perror("Failed to receive file");
        close(file_fd);
        return NULL;
    }

    printf("File received successfully\n");
    close(file_fd);
    return (void*)1;
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
 * Function that delete the client from his current chanel
 * 
 * @param dS socket descriptor of the client that is removed from all chanel
 * @return 1 if it was successful and 0 if there was any chanel where he was
*/
int removeClientChanel (int dS) {
    int i = 0;
    int res = 0;
    while (i<NB_CHANEL && res == 0) {
        int j = 0;
        while (j<NB_CLIENT_MAX) {
            if (tab_chanel[i].list_of_client[j] == dS){
                tab_chanel[i].list_of_client[j] = -1;
                res = 1;
            }
            j++;
        }
        i++;
    }
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
    send_message(dS_cible,"kick");
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
    close(getServeurdS());
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
    removeClientChanel(descripteur);
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
 * Lists the files in the server's 'biblio' directory and send the list to the client.
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

    send_message(descripteur, file_list);
    free(file_list);  // Free the memory allocated by update_file_list

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
    struct thread_argument argument = {dS};
    int i = pthread_create(&tid, NULL, file_recup_socket, filename);
    if (i != 0) {
        perror("Error creating the thread");
        return -1;
    }

    // Sends a message to the client to indicate that the file is being sent
    char notification[256];
    printf("file socket port vaut %s \n", file_socket.port);
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
    printf("Thread ended for file %s\n", filename);

    return 1;
}



/**
 * Function that create a usable chanel if there is enough place
 * 
 * @param chanel_name The name of the chanel
 * @return 1 if created and 0 if there isn't enough place for it 
*/
int createChanel (char* chanel_name) {
    if (strcmp(chanel_name,"") == 0) {
        return 0;
    }
    int i = 0;
    int res = 0;
    while (i<NB_CHANEL && !res) {
        if (strcmp(tab_chanel[i].name,"")==0) {
            tab_chanel[i].name = (char*)malloc(sizeof(chanel_name)+1);
            strcpy(tab_chanel[i].name,chanel_name);
            res = 1;
        }
        i++;
    }
    return res;
}

/**
 * Function that handle the add of a client to a chanel and remove from his previous one
 * 
 * @param dS The socket descriptor of the client who wants to be add
 * @param chanel_name The name of the chanel concerned
 * @return 1 if add and 0 if there isn't no place
 * 
*/
int ClientJoinChanel (int dS,char * chanel_name) {
    int add = 0;
    int del = 0;
    pthread_mutex_lock(&mutex_tab_chanel);
    int i = 0;
    while (i<NB_CHANEL && (add==0 || del == 0)) {
        if (strcmp(chanel_name, tab_chanel[i].name) == 0) {
            int j = 0;
            while (j<NB_CLIENT_MAX && add == 0) {
                if (tab_chanel[i].list_of_client[j]==-1 ){
                    tab_chanel[i].list_of_client[j]= dS;
                    add = 1;
                    puts("Client ajouté a un salon");
                }
                j++;;
            }
        }else {
            int m = 0;
            while (m<NB_CLIENT_MAX && del == 0) {
                if (tab_chanel[i].list_of_client[m] == dS){
                    tab_chanel[i].list_of_client[m] = -1;
                    del = 1;
                }
                m++;
            }
        }
        i++;
    }
    pthread_mutex_unlock(&mutex_tab_chanel);
    return add;
}

/**
 * This function handle the delete of a chanel 
 * 
 * @param chanel_name Name of the chanel concerned
 * @return 1 if the delete was successful and 0 if there wasn't any chanel corresponding 
*/
int deleteChanel (char* chanel_name) {
    int i = 0;
    int res = 0;
    while (i<NB_CHANEL && !res) {
        if (strcmp(tab_chanel[i].name,chanel_name)==0){
            for (int j = 0;j<NB_CLIENT_MAX;++j){
                tab_chanel[i].list_of_client[j] = -1;
            }
            tab_chanel[i].name = "";
            res = 1;
        }
        i++;
    }
    return res;
}

/**
 * Function that send the list of all existing chanel
 * 
 * @param dS The socket descriptor of the client who is aimed for the sending
 * @return 1 if all went good and -1 if an error occured
*/
int listAllChanel (int dS) {
    int size = 100*sizeof(char);
    int i = 0;
    char * message = (char*)malloc(size);
    while (i<NB_CHANEL) {
        if (strcmp(tab_chanel[i].name,"")!=0){
            int j = 0;
                if (size<strlen(message) + strlen(tab_chanel[i].name)) {
                    size = strlen(message) + strlen(tab_chanel[i].name)+100*sizeof(char);
                    char* temp = realloc(message,size);
                    if (temp == NULL) {
                        puts("Memory reallocation failed");
                        free(message);
                        return -1;
                    }
                    message = temp;
                }
                strcpy(message,tab_chanel[i].name);
                strcat(message, "\n");
                send_message(dS,message);
                free(message);
        }
        i++;
    }
    return 1;
}

/**
 * Function that send the list of all the chanel where the user is connected
 * 
 * @param dS The socket descriptor of the client who is aimed for the sending
 * @return 1 if all went good and -1 if an error occured
*/
int displayClientChanel (int dS) {
    int size = 100*sizeof(char);
    int i = 0;
    char * message = (char*)malloc(size);
    while (i<NB_CHANEL) {
        if (strcmp(tab_chanel[i].name,"")!=0){
            int j = 0;
            while (j<NB_CLIENT_MAX){
                if (tab_chanel[i].list_of_client[j] == dS) {
                    strcpy(message,tab_chanel[i].name);
                    strcat(message, "\n");
                    send_message(dS,message);
                    free(message);
                }
                j++;
            }
        }
        i++;
    }
    return 1;
}

/**
* This function is in charge of detection of commands in a message
* List of case value of return : 
*    - 1 if nothing to do (/kick)
*    - 2 if there is no command
*    - -1 if there is a problem 
*    - 0 if the user needs to be disconnected
* @param arg The message that is sent by the client 
* @param descripteur socket descriptor of the client that has sent the message
* @return 1 if nothing is to do, 0 if we need to shutdown and 2 if the classic behavior is expected
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
        }else if (strcmp(tok, "biblio") == 0) {
            return filelist(descripteur);
        }else if (strcmp(tok, "recup") == 0) {
            tok = strtok(NULL, " ");
            return file_recup_thread(descripteur, tok);
        }else if (strcmp(tok, "send") == 0) {
            tok = strtok(NULL, " ");
            file_reception(descripteur, tok);
        }else if (strcmp(tok, "createChanel") == 0) {
            tok = strtok(NULL, " ");
            return createChanel(tok);
        }else if (strcmp(tok, "joinChanel") == 0) {
            tok = strtok(NULL, " ");
            return ClientJoinChanel(descripteur,tok);
        }else if (strcmp(tok, "listChanel") == 0) {
            tok = strtok(NULL, " ");
            listAllChanel(descripteur);
        }else if (strcmp(tok, "myChanel") == 0) {
            tok = strtok(NULL, " ");
            displayClientChanel(descripteur);
        }else if (strcmp(tok, "deleteChanel") == 0) {
            tok = strtok(NULL, " ");
            deleteChanel(tok);
        }else  {
            return 2;
        }
    }else {return 2;}
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
            ClientJoinChanel(dS,"General");
            res = 0;
        }
        ++i;
    }
    pthread_mutex_unlock(&mutex_tab_cli);
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
        if (res == 0) { //Condition if the client has deconnected
            puts("Annulation de connexion d'un client");
            int resultat = delete_client(dS);
            close(dS);
            pthread_exit(NULL);
        }
        else if (res < 0) {// Condition if the connection encounter an error
            perror("Error receiving the nickname");
            exit(0);
        }
        else { //If the message is fine 
            printf("Pseudo recu : %s \n",message);
            compt = 0;
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