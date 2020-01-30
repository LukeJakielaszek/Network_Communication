#ifndef SERVER_UTIL_H
#define SERVER_UTIL_H

#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <map>
#include <iostream>
#include <string>

using namespace std;

int getlistenfd(char *port);

#endif /** SERVER_UTIL_H */