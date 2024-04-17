#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


//Command to launch this program : ./Serveur port
// Exemple : ./Serveur 3500
/*
This program should act as a relay of message 
This should start the conversation when 2 Clients are connected, capture the senders and the receiver and switch them, and end when "fin" is sended by someone
*/
struct thread_argument {
    int descripteur;
    int* tab_of_client;
};

int creation_socket() {
        int dS = socket(PF_INET, SOCK_STREAM, 0);
        printf("Socket created \n");
        return dS;
}

struct sockaddr_in param_socket_adresse(char * port) {
    //Get the adresses of the first Client
    struct sockaddr_in adresse1;
    adresse1.sin_family = AF_INET; //address family
    adresse1.sin_addr.s_addr = INADDR_ANY; //address to accept any incoming messages
    adresse1.sin_port = htons(atoi(port)); //port passed as argument
    printf("Adresse creation \n");
    return adresse1;
}

int make_bind(int socket,struct sockaddr_in adresse) {
    int connect = bind(socket,(struct sockaddr*)&adresse,sizeof(adresse)); //variable "connect" to avoid conflict with the function connect
    if (connect != 0) {
        printf("Connection error: bind failed\n");
        return connect;
    }
    printf("Socket named\n");
}

int connect_to_client (struct sockaddr_in adress, int descripteur) {
    socklen_t lenght = sizeof(struct sockaddr_in); //keep the length of the address
    int dSClient = accept(descripteur, (struct sockaddr*) &adress, &lenght); //keep the (new) socket descriptor of the client
    if (dSClient == -1) {
        perror("Connection error: client failed to connect\n");
        return -1;
    }
    printf("\n-- Client connected --\n");
    return dSClient;
}

void * discussion (void * arg) {
    struct thread_argument * argument = (struct thread_argument *) arg;
    short conversation = 1;
    char message[300];
    int receiver;
    if (argument->tab_of_client[0] == argument->descripteur) {
        receiver = argument->tab_of_client[1];
    }
    else {
        receiver = argument->tab_of_client[0];
    }
    while (conversation) {
        int res =  recv(argument->descripteur, message, sizeof(char)*300, 0);
        printf("message recu : %s \n",message);
        if (res < 0) {
            perror("Error receiving the message");
                exit(0);
        }

        res = send(receiver, message, sizeof(char)*300 , 0);
        if (res < 0) {
            perror("Error sending the message");
            exit(0);
        }
        sleep(0.01);
}
}

int main(int argc, char *argv[]) {

    if (argc != 2) { //security : check if the number of arguments is correct
        //printf("Number of arguments incorrect\n");
        perror("Incorrect number of arguments");
        printf("Usage : %s <port>\n", argv[0]);
        exit(0);
        return -1;
    }

    printf("Start program\n");

    short running = 1;    

    int dS = creation_socket();

    struct sockaddr_in adresse = param_socket_adresse(argv[1]);

    int connect = make_bind(dS,adresse);

    int tab_client[2]; 

    /*
    One of the problem is to find who is to konow who is the sender and who is the receiver for the start, given that this is alternante next
    The idea here is that the both clients send their type to be affected the correct role
    */
   //The part above is 
   //This loop should be running unless there is an external interruption, because this allow to handle the connection of new Clients when the last conversation has ended
    while (running) {

        int ecoute = listen(dS,1);
        if (ecoute < 0) {
            perror("Connection error: listen failed\n");
            return ecoute;
        }
        printf("Listening mode\n");

        struct sockaddr_in aC1 ;
        struct sockaddr_in aC2 ;

        tab_client[0] = connect_to_client(aC1,dS);
        tab_client[1] = connect_to_client(aC2,dS);

        pthread_t tid;
        pthread_t tid2;

        struct thread_argument arg1 = {tab_client[0],tab_client};
        struct thread_argument arg2 = {tab_client[1],tab_client};

        printf("Initialisation réussi \n") ;

        int i = pthread_create (&tid, NULL, discussion,&arg1);
        int j = pthread_create(&tid2,NULL,discussion,&arg2);

        //Waiting for the close of the 2 threads 
        pthread_join(tid,NULL);
        pthread_join(tid2,NULL);

}
}