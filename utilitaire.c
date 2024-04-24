#include "utilitaire.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

//A debugger

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

int recv_message(int descripteur,char** message) {
    int taille;
    int res =  recv(descripteur,&taille, sizeof(int), 0);
    if (res == 0) {
        return res;
    }
    if (res < 0) {
        //puts("Erreur reception de la taille");
        return res; //Return of error code
    }
    *message = (char*) malloc(sizeof(char)*(taille+1));
    res = recv(descripteur,*message,taille*sizeof(char),0);
    if (res < 0) {
        return res; //Return of error code
    }
    return 1; //All went good
}

int semaphore_wait (int sem) {
    struct sembuf waiting_buffer;
    waiting_buffer.sem_num = 0;
    waiting_buffer.sem_op = -1;
    waiting_buffer.sem_flg = 0;
    if (semop(sem,&waiting_buffer,1) == 0) {
        return 0;
    }
}

int semaphore_unlock (int sem) {
    struct sembuf unlocking_buffer;
    unlocking_buffer.sem_num = 0;
    unlocking_buffer.sem_op = 1;
    unlocking_buffer.sem_flg = 0;
    if (semop(sem,&unlocking_buffer,1) == 0){
        return 0;
    }
}