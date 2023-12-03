#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <limits.h>
#include "../commons/commons.h"
#include "../client/commands.h"
#include "./interface.h"

#define PORT 4000

void printUsage()
{
    fprintf(stderr, "usage: ./client <username> <hostname>\n");
    exit(EXIT_FAILURE);
}

struct hostent *getServerHost(char *hostname)
{
    struct hostent *server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }
    return server;
}

int createSocket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        fprintf(stderr, "ERROR opening socket\n");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void connectToServer(int sockfd, struct sockaddr_in serv_addr)
{
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "ERROR connecting\n");
        exit(EXIT_FAILURE);
    }
}

struct sockaddr_in initializeServerAddress(struct hostent *server)
{
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);
    return serv_addr;
}

int check_login_response(int socket)
{
    const packet_t *login_response_packet = receive_packet_from_socket(socket);
    int response = atoi(login_response_packet->payload);
    if (response == EXIT_SUCCESS)
    {
        printf("Logado com sucesso ao servidor!\n");
        return 0;
    }
    return ERROR;
}

void handle_inotify_event(int fd) {
    char buffer[4096];
    ssize_t bytesRead;

    bytesRead = read(fd, buffer, sizeof(buffer));
    if (bytesRead == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    for (char *ptr = buffer; ptr < buffer + bytesRead; ) {
        struct inotify_event *event = (struct inotify_event *)ptr;

        if (event->mask & IN_CLOSE_WRITE) {
            printf("File m_time has changed: %s\n", event->name);
            //DELETE(event->name);
            //UPLOAD(event->name);
            //inotify precisa de um Mutex para esse
            //  tipo de operaçao nao entrar em loop
            //  apagando o arquivo do client antes
            //  de ele ser upado para o server
        }

        if (event->mask & IN_CREATE) {
            printf("File created: %s\n", event->name);
            // UPLOAD(event->name);
        }
        if (event->mask & IN_MOVED_FROM) {
            printf("File moved from: %s\n", event->name);
            // DELETE(event->name);
        }
        if (event->mask & IN_MOVED_TO) {
            printf("File moved to: %s\n", event->name);
            // UPLOAD(event->name);
        }
        if (event->mask & IN_DELETE) {
            printf("File moved to: %s\n", event->name);
            // DELETE(event->name);
        }

        // Add more event checks if needed
        ptr += sizeof(struct inotify_event) + event->len;
    }
}

void start_inotify() {
    int inotifyFd, watchFd;

    // Initialize inotify
    inotifyFd = inotify_init();
    if (inotifyFd == -1) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    // Gets the sync_dir pathfile

    char *PATH;
    char currentPath[256];

    // Get the current working directory
    if (getcwd(currentPath, sizeof(currentPath)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the combined path
    size_t pathLength = strlen(currentPath) + strlen("/sync_dir") + 1;
    PATH = (char *)malloc(pathLength);

    // Check for allocation failure
    if (PATH == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Combine the paths
    strcpy(PATH, currentPath);
    strcat(PATH, "/sync_dir");

    // Now PATH contains the concatenated path
    printf("Concatenated Path: %s\n", PATH);

    // Add a watch for the directory
    watchFd = inotify_add_watch(inotifyFd, PATH, IN_MODIFY | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE | IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE);
    if (watchFd == -1) {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
    }

    printf("Watching for file modifications in the directory...\n");

    // Main event loop
    while (1) {
        handle_inotify_event(inotifyFd);
    }

    // Close inotify descriptor when done (this part will not be reached in this example)
    close(inotifyFd);
    // Don't forget to free the allocated memory
    free(PATH);
    return;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printUsage();
    }
    const char *username = argv[1];

    struct hostent *server = getServerHost(argv[2]);

    struct sockaddr_in serv_addr = initializeServerAddress(server);

    int sockfd = createSocket();

    connectToServer(sockfd, serv_addr);

    char *username_payload = strdup(username);

    if (username_payload == NULL)
    {
        perror("Error ao criar username payload.");
        exit(EXIT_FAILURE);
    }

    packet_t *packetUsername = create_packet(CMD_LOGIN, username_payload, strlen(username));

    if (send_packet_to_socket(sockfd, packetUsername) < 0)
    {
        perror("Error ao enviar username para o servidor.");
        free(username_payload);
        destroy_packet(packetUsername);
        exit(EXIT_FAILURE);
    }

    if (check_login_response(sockfd) < 0)
    {
        close(sockfd);
        printf("Conexão negada pelo servidor\n");
        return EXIT_FAILURE;
    }

    pthread_t userInterfaceThread;

    if (pthread_create(&userInterfaceThread, NULL, userInterface, (void *)&sockfd))
    {
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
