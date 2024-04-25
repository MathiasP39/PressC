#include "utilitaire.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

//A debugger

/**
 * Sends a message over a given descriptor.
 * 
 * @param descripteur The descriptor to send the message through.
 * @param message The message to be sent.
 * @return Returns 1 if the message was sent successfully, -1 otherwise.
 */
int send_message(int descripteur, char* message) {
    //Let's send the length
    int longueur = strlen(message);
    int res = send(descripteur,&longueur, sizeof(longueur), 0);
    if (res<0) {
        return -1;
    }
    //Catch some the error 
    //Let's send the message with the appropriate size
    res = send(descripteur,message,sizeof(char)*strlen(message),0); 
    if (res<0) {
        return -1;
    }
    //catch some error 
    return 1 ;//All went good
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
    int res = recv(descripteur, &taille, sizeof(int), 0);
    if (res == 0) {
        return res;
    }
    if (res < 0) {
        //puts("Erreur reception de la taille");
        return res; //Return of error code
    }
    *message = (char*) malloc(sizeof(char) * (taille + 1));
    res = recv(descripteur, *message, taille * sizeof(char), 0);
    if (res < 0) {
        return res; //Return of error code
    }
    return 1; //All went good
}

/**
 * @brief Waits for a semaphore to become available.
 *
 * This function waits for the specified semaphore to become available by decrementing its value.
 * If the semaphore is already locked, the function will block until it becomes available.
 *
 * @param sem The semaphore identifier.
 * @return 0 if the semaphore was successfully acquired, -1 otherwise.
 */
int semaphore_wait (int sem) {
    struct sembuf waiting_buffer;
    waiting_buffer.sem_num = 0;
    waiting_buffer.sem_op = -1;
    waiting_buffer.sem_flg = 0;
    if (semop(sem,&waiting_buffer,1) == 0) {
        return 0;
    }
    else {
        return -1;
    }
}

/**
 * @brief Unlocks a semaphore.
 *
 * This function unlocks a semaphore by performing a semaphore operation with a positive value.
 *
 * @param sem The semaphore identifier.
 * @return 0 if the semaphore was successfully unlocked, otherwise -1.
 */
int semaphore_unlock (int sem) {
    struct sembuf unlocking_buffer;
    unlocking_buffer.sem_num = 0;
    unlocking_buffer.sem_op = 1;
    unlocking_buffer.sem_flg = 0;
    if (semop(sem,&unlocking_buffer,1) == 0){
        return 0;
    }
    else {
        return -1;
    }
}