#include "utilitaire.h"

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