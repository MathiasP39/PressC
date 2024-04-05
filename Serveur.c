#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



int main(int argc, char *argv[]) {
    printf("Début programme\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    printf("Socket créé\n");

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(atoi(argv[1]));
    int connect = bind(dS, (struct sockaddr*)&ad, sizeof(ad)); //variable "connect" to avoid conflict with the function connect
    if (connect < 0) {
        printf("Connexion error : bind failed\n");
        return -1;
    }
    printf("Socket nommé\n");

    int ecoute = listen(dS, 7); //variable "ecoute" to avoid conflict with the function listen
    if (ecoute < 0) {
        printf("Connexion error : listen failed\n");
        return -1;
    }
    printf("Mode écoute\n");
}