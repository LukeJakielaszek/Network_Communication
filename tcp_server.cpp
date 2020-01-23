#include <iostream>
#include <string>

// custom scripts
#include "server_util.h"

const int BUFFSIZE = 100;

int main(int argc, char ** argv){
    std::cout << "SERVER" << std::endl;

    if(argc != 3){
        printf("ERROR: Invalid number of arguments [%d]\n", argc);
        return -1;
    }

    // get listener port
    char* port_to_listen_on = argv[1];

    // get file directory
    char* file_directory = argv[2];

    // server setup
    int listenfd;  // listener socket
    int connectedfd; // connected socket


    struct sockaddr_storage client_address;
    socklen_t client_size;
    char line[BUFFSIZE];
    ssize_t bytes_read;
    char clientName[BUFFSIZE];
    char clientPort[BUFFSIZE];

    // creates a passive socket to be used for accepting connections
    listenfd = getlistenfd(port_to_listen_on);

    // infinite loop to get connections
    while(1){
        // get size of client
        client_size = sizeof(client_address);

        printf("Attempting to connect...\n");

        // attempt to connect to a client
        connectedfd = accept(listenfd, (struct sockaddr*)&client_address, &client_size);

        // check for success
        if(connectedfd == -1){
            printf("ERROR: Unable to connect to client\n");
            continue;
        }

        printf("Connected\n");

        // gets information about client
        if (getnameinfo((struct sockaddr*)&client_address, client_size,
                clientName, BUFFSIZE, clientPort, BUFFSIZE, 0)!=0) {
            fprintf(stderr, "error getting name information about client\n");
        } else {
            printf("accepted connection from %s:%s\n", clientName, clientPort);
        }
    }
}