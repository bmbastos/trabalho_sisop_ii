#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#ifdef __linux__
#include <sys/inotify.h>
#endif
#include <limits.h>
#include "../commons/commons.h"
#include "../client/commands.h"
#include "./interface.h"

typedef struct thread_data
{
    struct sockaddr_in serv_addr;
    const char *username;
    int data_socket;
} thread_data_t;

void printUsage()
{
    printf("Invalid arguments.\nUsage: ./client <username> <server_ip_address> <port>\n");
    exit(EXIT_FAILURE);
}

struct INotifyThreadArgs {
    const char *username;
    int sockfd;
};

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

struct sockaddr_in initializeServerAddress(struct hostent *server, int port)
{
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
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

void handle_inotify_event(int fd, int sockfd, char* path)
{
#ifdef __linux__
    char buffer[4096];
    ssize_t bytesRead;

    bytesRead = read(fd, buffer, sizeof(buffer));
    if (bytesRead == -1)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }

    for (char *ptr = buffer; ptr < buffer + bytesRead;)
    {
        struct inotify_event *event = (struct inotify_event *)ptr;

        char currentPath[1024];
        strcpy(currentPath, path);

        if (event->mask & IN_CLOSE_WRITE)
        {
            printf("File m_time has changed: %s\n", event->name);
            // DELETE(event->name);
            // UPLOAD(event->name);
            // inotify precisa de um Mutex para esse
            //   tipo de operaçao nao entrar em loop
            //   apagando o arquivo do client antes
            //   de ele ser upado para o server
        }

        if (event->mask & IN_CREATE)
        {
            printf("File created: %s\n", event->name);
            strcat(currentPath, "/");
            strcat(currentPath, event->name);
            printf("RESULTED IN PATH: %s", currentPath);
            upload_file(currentPath, sockfd);
        }
        if (event->mask & IN_MOVED_FROM)
        {
            printf("File moved from: %s\n", event->name);
            delete_file(event->name, sockfd);
        }
        if (event->mask & IN_MOVED_TO)
        {
            printf("File moved to: %s\n", event->name);
            strcat(currentPath, "/");
            strcat(currentPath, event->name);
            printf("RESULTED IN PATH: %s", currentPath);
            upload_file(currentPath, sockfd);
        }
        if (event->mask & IN_DELETE)
        {
            printf("File moved to: %s\n", event->name);
            delete_file(event->name, sockfd);
        }

        ptr += sizeof(struct inotify_event) + event->len;
    }
#endif
}

// void *start_inotify(void *socket_ptr) {
void *start_inotify(void *threadArgsPtr) {    
    #ifdef __linux__
    struct INotifyThreadArgs *threadArgs = (struct INotifyThreadArgs *)threadArgsPtr;
    char username_array[strlen(threadArgs->username) + 1];
    strcpy(username_array, threadArgs->username);

    int socket = threadArgs->sockfd;

    printf("\n\nthe username is: %s\n", username_array);

    int inotifyFd, watchFd;

    // Initialize inotify
    inotifyFd = inotify_init();
    if (inotifyFd == -1)
    {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    // Gets the sync_dir pathfile

    char *PATH;
    char currentPath[256];

    // Get the current working directory
    if (getcwd(currentPath, sizeof(currentPath)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the combined path
    size_t pathLength = strlen(currentPath) + strlen("/sync_dir_") + strlen(username_array) + 1;
    PATH = (char *)malloc(pathLength);

    // Check for allocation failure
    if (PATH == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // printf("Current Path: %s\n", currentPath);
    // printf("Current Path Length: %ld\n", strlen(currentPath));

    // Combine the paths
    strcpy(PATH, currentPath);
    strcat(PATH, "/sync_dir_");
    strcat(PATH, username_array);

    // Now PATH contains the concatenated path
    printf("Concatenated Path: %s\n", PATH);

    // Add a watch for the directory
    watchFd = inotify_add_watch(inotifyFd, PATH, IN_MODIFY | IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE);
    if (watchFd == -1)
    {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
    }

    printf("Watching for file modifications in the directory...\n");

    // Main event loop
    while (1)
    {
        handle_inotify_event(inotifyFd, socket, PATH);
    }

    // Close inotify descriptor when done (this part will not be reached in this example)
    close(inotifyFd);
    // Don't forget to free the allocated memory
    free(PATH);
#endif
    return NULL;
}

void *watch_server_changes(void *data_arg)
{
    thread_data_t *data = (thread_data_t *)data_arg;
    int notification_socket = createSocket();
    connectToServer(notification_socket, data->serv_addr);
    packet_t *packet_watch = create_packet(CMD_WATCH_CHANGES, data->username, strlen(data->username) + 1);
    packet_t *packet_buffer = malloc(sizeof(packet_t));
    if (packet_buffer == NULL)
    {
        perror("ERROR allocating memory for packet\n");
        return (void *)-1;
    }

    if (send_packet_to_socket(notification_socket, packet_watch) < 0)
    {
        perror("Failed to send watch changes socket");
    }

    while (1)
    {
        bzero(packet_buffer, sizeof(packet_t));
        packet_buffer = receive_packet_wo_payload(notification_socket);
        if (receive_packet_payload(notification_socket, packet_buffer) < 0)
        {
            perror("Failed to receive packet payload");
            continue;
        }

        if (!packet_buffer)
        {
            printf("Error reading changes notification from socket\n");
            continue;
        }

        if (packet_buffer->type != CMD_NOTIFY_CHANGES)
        {
            printf("Received unexpected packet. Expected CMD_NOTIFY_CHANGES\n");
            continue;
        }
        else
        {
            download_file(packet_buffer->payload, data->data_socket, 1);
        }
    }
}

void get_sync_dir(const char *username, int sockfd, char *username_payload, packet_t *packetUsername) {
    char username_array[strlen(username) + 1];
    strcpy(username_array, username);

    create_folder(username_array);

    struct INotifyThreadArgs threadArgs;
    threadArgs.username = username;
    threadArgs.sockfd = sockfd;

    // THREAD INOTIFY
    pthread_t start_inotifyThread;

    if (pthread_create(&start_inotifyThread, NULL, start_inotify, (void *)&threadArgs))
    {
        fprintf(stderr, "Erro ao criar thread start_inotify.\n");
        free(username_payload);
        destroy_packet(packetUsername);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printUsage();
    }
    const char *username = argv[1];

    struct hostent *server = getServerHost(argv[2]);

    int port = atoi(argv[3]);
    struct sockaddr_in serv_addr = initializeServerAddress(server, port);

    int sockfd = createSocket();

    connectToServer(sockfd, serv_addr);

    char *username_payload = strdup(username);

    if (username_payload == NULL)
    {
        perror("Error ao criar username payload.");
        exit(EXIT_FAILURE);
    }

    packet_t *packetUsername = create_packet(CMD_LOGIN, username_payload, strlen(username) + 1);

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

    get_sync_dir(username, sockfd, username_payload, packetUsername);

    pthread_t userInterfaceThread;

    if (pthread_create(&userInterfaceThread, NULL, userInterface, (void *)&sockfd))
    {
        fprintf(stderr, "Erro ao criar thread userInterface.\n");
        free(username_payload);
        destroy_packet(packetUsername);
        exit(EXIT_FAILURE);
    }

    pthread_t server_changes_thread;

    thread_data_t *notification_data = malloc(sizeof(thread_data_t));
    notification_data->data_socket = sockfd;
    notification_data->serv_addr = serv_addr;
    notification_data->username = username;

    if (pthread_create(&server_changes_thread, NULL, watch_server_changes, (void *)notification_data))
    {
        fprintf(stderr, "Erro ao criar thread userInterface.\n");
        free(username_payload);
        destroy_packet(packetUsername);
        exit(EXIT_FAILURE);
    }

    pthread_join(userInterfaceThread, NULL);

    close(sockfd);

    free(username_payload);
    free(notification_data);
    destroy_packet(packetUsername);

    return 0;
}
