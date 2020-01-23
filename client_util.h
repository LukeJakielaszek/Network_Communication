#ifndef CLIENT_UTIL_H
#define CLIENT_UTIL_H

#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 

int connect_to_server(int port, const char *ip_address);

#endif /** CLIENT_UTIL_H */