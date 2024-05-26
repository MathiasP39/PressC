#ifndef SERVEUR_CONNECTION
#define ANY_UNIQUE_NAME_HERE

#include <netinet/in.h> 

typedef struct {
    int dS_Server;
    struct sockaddr_in server_address;
} server_info;

typedef struct {
    int socket;
    struct sockaddr_in adresse;
    char* port;
}  socket_info;
 

extern socket_info file_socket;

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

int get_free_port(int server_port);

/**
 * Creates a socket for file recovery.
 * 
 * @param server_port The port number of the server.
 * @return The socket information structure.
 */
socket_info create_file_recup_socket(int server_port);

/**
 * Sends a file to a client.
 * 
 * @param info The socket information structure.
 * @param filename The name of the file to send.
 * @return 1 if the file is successfully sent, -1 otherwise.
*/
void* file_recup_socket(void* arg);

#endif