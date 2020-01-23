#include <iostream>
#include <string>

// custom scripts
#include "client_util.h"

int main(int argc, char ** argv){
    std::cout << "CLIENT" << std::endl;

    if(argc != 5){
        printf("ERROR: Invalid number of arguments [%d]\n", argc);
        return -1;
    }

    std::string server_host = argv[1];
    std::string server_port = argv[2];
    std::string file_name = argv[3];
    std::string directory = argv[4];

    int connected_socket = connect_to_server(atoi(server_port.c_str()), server_host.c_str());

    
}