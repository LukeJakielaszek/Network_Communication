#include <iostream>
#include <string>

// custom scripts
#include "client_util.h"

using namespace std;

const int BUFFSIZE = 1024;

int main(int argc, char ** argv){
    if(argc != 5){
        printf("ERROR: Invalid number of arguments [%d]\n", argc);
        printf("\t./tcp_client server_host server_port file_name directory\n");
        return -1;
    }

    string server_host = argv[1];
    string server_port = argv[2];
    string file_name = argv[3];
    string directory = argv[4];

    int connected_socket = connect_to_server(atoi(server_port.c_str()), server_host.c_str());

    // send the file name
    send(connected_socket , file_name.c_str(), file_name.size(), 0); 
    printf("request message sent [%s]\n", file_name.c_str()); 

    unsigned long long int file_size;
    read(connected_socket, &file_size, sizeof(file_size));

    cout << "FILE SIZE [" << file_size << "]" << endl;

    // buffer to incrementally store server response
    char buffer[BUFFSIZE];

    int valread;
    do{
        // receive file from the connected socket
        valread = read(connected_socket, buffer, BUFFSIZE-1);
        buffer[valread] = '\0';
        printf("[%d] : [%s]\n", valread, buffer);

        file_size -= valread;
    }while(file_size > 0);
}