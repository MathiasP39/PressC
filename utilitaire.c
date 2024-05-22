#include "utilitaire.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
// File management
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>




/**
 * Sends a message over a given descriptor.
 * 
 * @param descripteur The descriptor to send the message through.
 * @param message The message to be sent.
 * @return Returns 1 if the message was sent successfully, -1 otherwise.
 */
int send_message(int descripteur, char* message) {
    int longueur = strlen(message);
    if (longueur > 500) { //Security : if the message is too big, we don't receive it
        //puts("send_message : Message trop grand");
        return -1;
    }

    int res = send(descripteur,&longueur, sizeof(longueur), 0); //Send the size of the message to send
    if (res < 0) {
        return -1;
    }

    res = send(descripteur,message,sizeof(char)*strlen(message),0); //Send the message
    if (res<0) {
        return -1;
    }
    return 1 ; //All went good
}

/**
 * Receives a message from a socket descriptor.
 * 
 * @param descripteur The socket descriptor to receive the message from.
 * @param message A pointer to a char pointer that will store the received message.
 * 
 * @return Returns 1 if the message is successfully received, 0 if the connection is closed, or a negative value if an error occurs.
 */
int recv_message(int descripteur, char** message) {
    int taille;

    int res = recv(descripteur, &taille, sizeof(int), 0); //Reception of the size of the message to receive
    if (res == 0) { //End of the connection
        return res;
    }
    if (res < 0) {
        //puts("recv_message : Erreur reception de la taille");
        return res; //Return of error code
    }

    if (*message != NULL) { //Is useful if the function is called multiple times : free the memory before reallocating it
        free(*message);
    }

    *message = (char*) malloc(sizeof(char) * (taille + 1)); //Allocation of the memory for the message
    if (*message == NULL) {
        //puts("recv_message : Erreur allocation memoire");
        return -1; //Return of error code
    }

    (*message)[taille] = '\0'; //End of the string (security)

    res = recv(descripteur, *message, taille * sizeof(char), 0); //Reception of the message
    if (res < 0) {
        //puts("recv_message : Erreur reception du message");
        return res; //Return of error code
    }
    return 1; //All went good
}


//FILE MANAGEMENT


/**
 * Updates the file list in the specified directory.
 * 
 * @param directory The directory path.
 * @return A string of filenames separated by newlines, or NULL if an error occurs.
 */
char* update_file_list(const char* directory) {
    DIR* dir;
    struct dirent* entry;
    char* file_list = malloc(1);  // Start with an empty string
    *file_list = '\0';

    dir = opendir(directory); // Open the directory
    if (dir == NULL) {
        perror("Unable to open directory");
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Ignore the "." and ".." entries
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            file_list = realloc(file_list, strlen(file_list) + strlen(entry->d_name) + 2);  // +2 for the newline and null terminator
            strcat(file_list, entry->d_name); // Add the filename to the list
            strcat(file_list, "\n");
        }
    }

    closedir(dir);
    return file_list;
}

/**
 * Sends a file over a given descriptor.
 * 
 * @param descripteur The descriptor to send the file through.
 * @param file_name The name of the file to be sent.
 * @return Returns 1 if the file was sent successfully, -1 otherwise.
*/
int send_file(int descripteur, const char* file_name) {
    struct stat file_stat;
    int file_fd = open(file_name, O_RDONLY); // Open the file

    if (file_fd == -1) {
        perror("Failed to open file");
        return -1;
    }

    if (fstat(file_fd, &file_stat) == -1) { // Get the file stats (attributes)
        perror("Failed to get file stats");
        close(file_fd);
        return -1;
    }

    ssize_t bytes_sent = sendfile(descripteur, file_fd, NULL, file_stat.st_size); // Send the file
    close(file_fd);

    if (bytes_sent == -1) {
        perror("Failed to send file");
        return -1;
    }

    return 1;
}