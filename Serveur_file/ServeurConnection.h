#ifndef SERVEUR_CONNECTION
#define ANY_UNIQUE_NAME_HERE

#include <netinet/in.h> 

typedef struct {
    int dS_Server;
    struct sockaddr_in server_address;
} server_info;

server_info setup_socket(char* port);

/**
 * Connects to a client.
 * 
 * This function accepts a client connection on the given socket descriptor.
 * It returns the new socket descriptor of the connected client.
 * 
 * @param adress The server address structure.
 * @param descripteur The socket descriptor of the server.
 * @return The new socket descriptor of the connected client, or -1 if an error occurred.
 */
int connect_to_client(server_info params);

#endif