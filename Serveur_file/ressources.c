#include <sys/sem.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include "ressources.h"

sem_t static semaphore_nb_client;

int static Serveur_dS ;


int setServeurDS (int dS) {
    Serveur_dS = dS;
}

int init_ressources (int dS) {
    int res = sem_init(&semaphore_nb_client,0,NB_CLIENT_MAX);
    if (res < 0) {
        perror("Init of semaphore, error while initialising");
        exit(-1);
    }

    setServeurDS(dS);

}

int try_lock_sem_nb_client () {
    int res = sem_trywait(&semaphore_nb_client);
    return res;
}

int release_sem_nb_client () {
    int res = sem_post(&semaphore_nb_client);
    return res;
}

int getServeurdS () {
    return Serveur_dS;
}
