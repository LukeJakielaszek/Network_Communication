#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>

#define BACKLOG 10

#define EXIT_GETADDRINFO_ERROR 2
#define EXIT_BIND_FAILURE 3
#define EXIT_LISTEN_FAILURE 4

int getlistenfd(char *port);

// gets listener file descriptor from port number
int getlistenfd(char *port){
  int listenfd, status;

  struct addrinfo hints, *res, *p;

  memset(&hints, 0, sizeof(hints)); // sets hints to 0s

  hints.ai_socktype = SOCK_STREAM; // For TCP connection
  hints.ai_family = AF_INET; // for IPv4 connection type

  // gets internet address information
  status = getaddrinfo(NULL, port, &hints, &res);
  if(status != 0){
    printf("ERROR: Get Address Info erro %s\n", gai_strerror(status));
    exit(EXIT_GETADDRINFO_ERROR);
  }

  // loops through address info list
  for(p = res; p != NULL; p = p->ai_next){
    // attempt to create an endpoint for communication
    listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    // if unable to create socket
    if(listenfd < 0){
      continue;
    }

    // assigns address to socket
    if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0){
      break;
    }
  }

  // frees address
  freeaddrinfo(res);

  // ensures bind succeeded
  if(p == NULL){
    printf("ERROR: Failed to bind.\n");
    exit(EXIT_BIND_FAILURE);
  }

  // marks the socket as an acceptor
  if(listen(listenfd, BACKLOG) < 0){
    printf("ERROR: Failed to repurpose socket as a listener.\n");
    exit(EXIT_LISTEN_FAILURE);
  }

  // return the passive socket
  return listenfd;
}