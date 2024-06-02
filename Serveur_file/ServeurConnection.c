#include "ServeurConnection.h"
#include "../lib_headers/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

socket_info file_socket;

/**
 * Creates a socket for TCP/IP communication.
 *
 * @return The socket file descriptor if successful, -1 otherwise.
 */
int creation_socket() {
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1) {
        perror("Connection error: socket creation failed\n");
        return dS;
    }
    printf("Socket created \n");
    return dS;
}

/**
 * Creates and returns a socket address structure based on the given port number.
 *
 * @param port The port number to be used for the socket address.
 * @return The created socket address structure.
 */
struct sockaddr_in param_socket_adresse(char *port) {
    // Get the addresses of the first Client
    struct sockaddr_in adresse1;
    adresse1.sin_family = AF_INET; // address family
    adresse1.sin_addr.s_addr = INADDR_ANY; // address to accept any incoming messages
    adresse1.sin_port = htons(atoi(port)); // port passed as argument
    return adresse1;
}

/**
 * Binds a socket to a specific address.
 *
 * @param socket The socket to bind.
 * @param adresse The address to bind the socket to.
 * @return Returns 0 on success, or an error code on failure.
 */
int make_bind(int socket, struct sockaddr_in adresse) {
    int connect = bind(socket, (struct sockaddr*)&adresse, sizeof(adresse)); //variable "connect" to avoid conflict with the function connect
    if (connect != 0) {
        perror("Connection error: bind failed");
        return connect;
    }
    printf("Socket named\n");
    return connect;
}


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
int connect_to_client(server_info info) {
    socklen_t lenght = sizeof(struct sockaddr_in); //keep the length of the address
    int dSClient = accept(info.dS_Server, (struct sockaddr*) &info.server_address, &lenght); //keep the (new) socket descriptor of the client
    if (dSClient == -1) {
        perror("Connection error: client failed to connect\n");
        return -1;
    }
    printf("\n-- Client connected --\n");
    return dSClient;
}

server_info setup_socket(char* port) {
    int res = 0;
    int dS = creation_socket();
    struct sockaddr_in adresse = param_socket_adresse(port);
    res = make_bind(dS, adresse);
    if (res < 0) {
        perror("Binding, Error connecting making the socket ready");
    }
    server_info info;
    info.dS_Server = dS;
    info.server_address = adresse;
    return info;
}

/**
 * Finds a free port on the server.
 * 
 * @return The first free port found, or -1 if no free port is found.
 */
int get_free_port(int server_port) {
    int port = server_port + 1; // Start from the server port + 1
    char port_str[6]; // Buffer to hold the port number as a string

    while (port < 65535) {
        int test_socket = socket(PF_INET, SOCK_STREAM, 0);
        sprintf(port_str, "%d", port); // Convert the port number to a string
        struct sockaddr_in adresse = param_socket_adresse(port_str);
        if (bind(test_socket, (struct sockaddr*)&adresse, sizeof(adresse)) == 0) {
            close(test_socket);
            printf("Found free port: %d\n", port);
            return port;
        }
        close(test_socket);
        port++;
    }
    return -1; // No free port found
}

/**
 * Creates a socket for file recovery.
 * 
 * @param server_port The port number of the server.
 * @return The socket information structure.
 */
socket_info create_file_recup_socket(int server_port) {
    socket_info info;
    info.socket = socket(PF_INET, SOCK_STREAM, 0);

    int port_num = get_free_port(server_port);
    char* port = malloc(6 * sizeof(char)); // Maximum 5 digits for a port number + null terminator
    if (port == NULL) {
        perror("Failed to allocate memory for port");
        return info;
    }

    sprintf(port, "%d", port_num);
    if (port == "-1") {
        close(info.socket);
        info.socket = -1;
        return info;
    }
    info.port = port;

    info.adresse = param_socket_adresse(port);
    if (make_bind(info.socket, info.adresse) != 0) {
        close(info.socket);
        info.socket = -1;
        return info;
    }

    if (listen(info.socket, 1) < 0) {
        close(info.socket);
        info.socket = -1;
        return info;
    }
    printf("File recovery socket created on port %s\n", port);

    return info;
}


/**
 * Sends a file to a client.
 * 
 * @param info The socket information structure.
 * @param filename The name of the file to send.
 * @return 1 if the file is successfully sent, -1 otherwise.
*/
void* file_recup_socket(void* arg) {
    char* filename = (char*)arg;
    socket_info info = file_socket; // Get the file sending socket information

    if (info.socket == -1) {
        return (void*)-1;
    }

    server_info info_server; 
    info_server.dS_Server = info.socket;
    info_server.server_address = info.adresse;
    int dSClient = connect_to_client(info_server);
    if (dSClient == -1) {
        return (void*)-1;
    }

    char filepath[256];
    sprintf(filepath, "./biblio/%s", filename);

    if (send_file(dSClient, filepath) == -1) {
        return (void*)-1;
    }
    printf("File %s sent\n", filename);

    // After sending the file
    if (shutdown(dSClient, SHUT_RDWR) == -1) {
        perror("Failed to shutdown the socket");
        return (void*)-1;
    }
    printf("Client disconnected from file recup\n");

    return (void*)1;
}