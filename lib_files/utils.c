#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "../lib_headers/utils.h"

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

