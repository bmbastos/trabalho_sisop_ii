#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include "../commons/commons.h"
#include "../client/commands.h"
#include "./interface.h"

#define PORT 4000

void printUsage() {
    fprintf(stderr, "usage: ./client <username> <hostname>\n");
    exit(EXIT_FAILURE);
}

struct hostent* getServerHost(char *hostname) {
    struct hostent *server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }
    return server;
}

int createSocket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "ERROR opening socket\n");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void connectToServer(int sockfd, struct sockaddr_in serv_addr) {
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR connecting\n");
        exit(EXIT_FAILURE);
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
    if (argc < 3) {
        printUsage();
    }
    const char *username = argv[1];

    struct hostent *server = getServerHost(argv[2]);
    
    struct sockaddr_in serv_addr = initializeServerAddress(server);
    
    int sockfd = createSocket();
    
    connectToServer(sockfd, serv_addr);

    char *username_payload = strdup(username);

    if (username_payload == NULL) {
        perror("Error ao criar username payload.");
        exit(EXIT_FAILURE);
    }

    printf("the username payload is: %s\n", username_payload);

    packet_t* packetUsername = createPacket(CMD_LOGIN, (uint32_t)strlen(username), username_payload);

    if (send_packet(sockfd, packetUsername) < 0) {
        perror("Error ao enviar username para o servidor.");
        free(username_payload);
        destroy_packet(packetUsername);
        exit(EXIT_FAILURE);
    }

    pthread_t userInterfaceThread;

    if(pthread_create(&userInterfaceThread, NULL, userInterface, (void *) &sockfd)) {
        fprintf(stderr, "Erro ao criar thread.\n");
        free(username_payload);
        destroy_packet(packetUsername);
        exit(EXIT_FAILURE);
    }

    pthread_join(userInterfaceThread, NULL);

    close(sockfd);

    free(username_payload);
    destroy_packet(packetUsername);

    return 0;
}
