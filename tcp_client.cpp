#include <iostream>
#include <string>
#include <fstream>

// custom scripts
#include "client_util.h"

using namespace std;

// read size for socket
const int BUFFSIZE = 1024;

// indicator for a file which does not exist
long long int DNE_FILE = -9999;

int main(int argc, char ** argv){
    if(argc != 5){
        printf("ERROR: Invalid number of arguments [%d]\n", argc);
        printf("\t./tcp_client server_host server_port file_name directory\n");
        return -1;
    }

    // get init params from user
    string server_host = argv[1];
    string server_port = argv[2];
    string file_name = argv[3];
    string directory = argv[4];

    // convert the port to an int
    int port = atoi(server_port.c_str());
    if(port < 0){
        printf("ERROR: Invalid port detected [%d]\n", port);
        return -1;
    }

    // connect to the server
    int connected_socket = connect_to_server(port, server_host.c_str());
    if(connected_socket < 0){
        printf("ERROR: Failed to make initial connection to server\n");
        return -1;
    }

    // send the file name
    if(send(connected_socket , file_name.c_str(), file_name.size(), 0) < 0){
        printf("ERROR: Failed to send server file [%s] request\n", file_name.c_str());
        close(connected_socket);
        return -1;
    }

    // used to hold the file size
    long long int file_size;

    // get the size of the requested file
    if(read(connected_socket, &file_size, sizeof(file_size)) < 0){
        printf("Error: Failed to receive file size from server\n");
        close(connected_socket);
        return -1;
    }

    // check if file exists on server side
    if(file_size == DNE_FILE){
        printf("File [%s] does not exist in the server\n", file_name.c_str());
        return 0;
    }

    // buffer to incrementally store server response
    char buffer[BUFFSIZE];

    // absolute path to file for client
    string absolute_path = directory + "/" + file_name;

    // open the output file
    ofstream out_file;
    out_file.open(absolute_path, ios::out | ios::binary | ios::trunc);

    if(out_file.is_open()){
        // output file successfully open
        do{
            // receive file from the connected socket
            int valread = read(connected_socket, buffer, BUFFSIZE-1);
            
            // check if we can still successfully read from server
            if(valread < 0){
                printf("Error: Connection failed during file transmission\n");
                close(connected_socket);
                out_file.close();
                return -1;
            }

            // add a null (used for printing while debugging)
            buffer[valread] = '\0';

            // write to the file
            out_file.write(buffer, valread);

            // check for write success
            if(!out_file.good()){
                printf("ERROR: Failed to write transmitted contents to output\n");
                close(connected_socket);
                out_file.close();
                return -1;
            }

            // decrement the number of bytes left to read
            file_size -= valread;
        }while(file_size > 0);
    }else{
        // output file failed to open
        printf("ERROR: Failed to open [%s]\n", absolute_path.c_str());
    }

    // close socket
    close(connected_socket);

    // close file
    out_file.close();

    // indicate successful file copy
    printf("File [%s] saved\n", file_name.c_str());
    
    return 0;
}