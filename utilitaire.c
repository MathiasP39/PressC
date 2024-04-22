#include "utilitaire.h"


int send_message(int descripteur, char* message) {
    //Let's send the length
    puts("Passage dans send");
    int longueur = strlen(message);
    int res = send(descripteur,&longueur, sizeof(longueur), 0);
    printf("La taille de longueur est : %ld",sizeof(longueur));
    puts("Envoie réussi");
    //Catch some the error 
    //Let's send the message with the appropriate size
    res = send(descripteur,message,sizeof(char)*strlen(message),0); 
    //catch some error 
    return 0 ;//All went good
}

int recv_message(int descripteur,char** message) {
    int * taille;
    printf("La taille d'un entier vaut : %ld",sizeof(int));
    int res =  recv(descripteur, taille, sizeof(int), 0);
    printf("Le res vaut %d : ",res);
    if (res < 0) {
        puts("Erreur reception de la taille");
    }
    else {
        puts("Reception de l'entier");
    }
    *message = (char*) malloc(sizeof(char)*(*taille+1));
    res = recv(descripteur,*message,*taille*sizeof(char),0);
    return 0; //All went good
}