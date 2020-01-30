#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 

// attempt to connect to a server using the port and ip address
int connect_to_server(int port, const char *ip_address){
    // client socket
    int client_socket = 0;
    struct sockaddr_in serv_addr; 

    // attempt to create the socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("ERROR: Failed to create socket\n"); 
        return -1; 
    } 

    // initialize connection params
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port);

    // converts IP address to binary format
    if(inet_pton(AF_INET, ip_address, &serv_addr.sin_addr)<=0)  
    { 
        printf("ERROR: Invalid host detected [%s]\n", ip_address);
        return -1; 
    } 
   
    // attempt to connect to server
    if (connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("ERROR: Failed to connect to server\n"); 
        return -1;
    }

    return client_socket;
}