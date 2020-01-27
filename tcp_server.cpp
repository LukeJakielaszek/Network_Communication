#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>

// custom scripts
#include "server_util.h"

using namespace std;

char * retrieve_file(const string &file_name, const string &directory, map<string, char*> &cache);

const int BUFFSIZE = 100;

int main(int argc, char ** argv){
    cout << "SERVER" << endl;

    if(argc != 3){
        printf("ERROR: Invalid number of arguments [%d]\n", argc);
        printf("\t./tcp_server port_to_listen_on file_directory\n");
        return -1;
    }

    // get listener port
    char* port_to_listen_on = argv[1];

    // get file directory
    char* file_directory = argv[2];

    // server setup
    int listenfd;  // listener socket
    int connectedfd; // connected socket
    map<string, char*> cache = get_init_map();

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

        // read from the current client
        char buffer[1024];
        int valread = read(connectedfd, buffer, 1024); 
        buffer[valread] = '\0';
        printf("[%d] : [%s]\n", valread, buffer);

        // send file via response
        string ret_file = retrieve_file(buffer, file_directory, cache);
        send(connectedfd, ret_file.c_str(), ret_file.size(), 0 ); 
        printf("response message sent\n"); 
    }
}

char * retrieve_file(const string &file_name, const string &directory, map<string, char*> &cache){
    // file returned to client
    char * ret_file;

    // check if cache contains our file
    auto iter = cache.find(file_name);

    if(iter == cache.end()){
        // if not find and attempt to cache file

        // construct the absolute path to the file
        string absolute_path = directory + "/" + file_name;

        // check if file exists within the defined directory
        ifstream client_file;
        client_file.open(absolute_path);

        if(client_file.is_open()){
            // if it does exist insert into cache


            // close the file
            client_file.close();
        }else{
            // else return null

        }
    }else{
        // simply return the cached file
        ret_file = iter->second;
    }

    return ret_file;
}