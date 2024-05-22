#ifndef RESSOURCE
#define ANY_UNIQUE_NAME_HERE
#include <semaphore.h>
#include <pthread.h>

#define NB_CLIENT_MAX 10

int init_ressources (int dS);

int try_lock_sem_nb_client ();

int release_sem_nb_client ();

int getServeurdS ();

#endif