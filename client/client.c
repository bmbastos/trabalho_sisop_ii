#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "../commons/commons.h"
#include "./interface.h"

#define PORT 4000

void printUsage() {
    fprintf(stderr, "usage: ./client <hostname>\n");
    exit(0);
}

struct hostent* getServerHost(char *hostname) {
    struct hostent *server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    return server;
}

int createSocket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "ERROR opening socket\n");
        exit(0);
    }
    return sockfd;
}

void connectToServer(int sockfd, struct sockaddr_in serv_addr) {
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR connecting\n");
        exit(0);
    }
}

struct sockaddr_in initializeServerAddress(struct hostent *server) {
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);
    return serv_addr;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printUsage();
    }

    struct hostent *server = getServerHost(argv[1]);
    struct sockaddr_in serv_addr = initializeServerAddress(server);

    while (1)
    {
      int sockfd = createSocket();
      connectToServer(sockfd, serv_addr);
      userInterface(sockfd);
      close(sockfd);
    }
    
    return 0;
}
