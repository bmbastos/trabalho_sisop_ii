#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/stat.h>
#include <limits.h>
#include <dirent.h>
#ifdef __linux__
#include <sys/inotify.h>
#endif
#include "../commons/commons.h"
#include "../client/commands.h"
#include "./interface.h"

struct sockaddr_in serv_addr;
int data_socket;
int get_sync_dir(char *username, int data_socket);
void delete_local_file(const char *filename, const char *username, const char *filepath);

void printUsage()
{
    printf("Invalid arguments.\nUsage: ./client <username> <server_ip_address> <port>\n");
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
    int data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket == -1)
    {
        fprintf(stderr, "ERROR opening socket\n");
        exit(EXIT_FAILURE);
    }
    return data_socket;
}

void connectToServer(int data_socket, struct sockaddr_in serv_addr)
{
    if (connect(data_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
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
    packet_t *login_response_packet = receive_packet_from_socket(socket);
    int response = atoi(login_response_packet->payload);
    if (response == EXIT_SUCCESS)
    {
        destroy_packet(login_response_packet);
        return 0;       // Logado com sucesso
    }
    destroy_packet(login_response_packet);
    return ERROR;
}

void handle_inotify_event(int fd, int data_socket, char* path, const char *username)
{
#ifdef __linux__
    ssize_t bytesRead;
    char *buffer = NULL;
    size_t bufferSize = 0;
    size_t totalBytesRead = 0;
    size_t chunkSize = 4096;

    do {
        buffer = realloc(buffer, bufferSize + chunkSize);
        if (buffer == NULL) {
            free(buffer);
            exit(EXIT_FAILURE);
        }

        bytesRead = read(fd, buffer + totalBytesRead, chunkSize);
        if (bytesRead == -1) {
            free(buffer);
            exit(EXIT_FAILURE);
        }

        totalBytesRead += bytesRead;
        bufferSize += chunkSize;

    } while (bytesRead == chunkSize);

    for (char *ptr = buffer; ptr < buffer + bytesRead;)
    {
        struct inotify_event *event = (struct inotify_event *)ptr;

        char currentPath[1024];
        strcpy(currentPath, path);

        if (event->mask & IN_CLOSE_WRITE)
        {
            printf("IN_CLOSE_WRITE\n");
            strcat(currentPath, "/");
            strcat(currentPath, event->name);
            upload_file(currentPath, data_socket);
        }

        if (event->mask & IN_CREATE)
        {
            printf("IN_CREATE\n");
            strcat(currentPath, "/");
            strcat(currentPath, event->name);
            upload_file(currentPath, data_socket);
        }
        if (event->mask & IN_MOVED_FROM)
        {
            printf("IN_MOVED_FROM\n");
            delete_file(event->name, data_socket, username);
        }
        if (event->mask & IN_MOVED_TO)
        {
            printf("IN_MOVED_TO\n");
            strcat(currentPath, "/");
            strcat(currentPath, event->name);
            upload_file(currentPath, data_socket);
        }
        if (event->mask & IN_DELETE)
        {
            printf("IN_DELETE\n");
            delete_file(event->name, data_socket, username);
        }

        ptr += sizeof(struct inotify_event) + event->len;
    }

    free(buffer);
#endif
}

void get_client_file_array(const char *basepath, char filenames[][256], int *file_count) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(basepath);
    if (dir == NULL) {
        return;
    }

    *file_count = 0;

    while ((entry = readdir(dir)) != NULL && *file_count < 100) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            // Copia o nome do arquivo para o array de nomes de arquivos
            snprintf(filenames[*file_count], sizeof(filenames[*file_count]), "%s", entry->d_name);
            (*file_count)++;
        }
    }
    closedir(dir);
}

void *handleInitialSync(void *threadArgsPtr)
{
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)threadArgsPtr;
    char username_array[strlen(threadArgs->username) + 1];
    strcpy(username_array, threadArgs->username);

    char currentPath[256];

    if (getcwd(currentPath, sizeof(currentPath)) == NULL)
    {
        exit(EXIT_FAILURE);
    }

    size_t pathLength = strlen(currentPath) + strlen("/sync_dir_") + strlen(username_array) + 1;
    
    char* PATH;
    PATH = (char *)malloc(pathLength);

    if (PATH == NULL)
    {
        exit(EXIT_FAILURE);
    }

    sprintf(PATH, "%s/sync_dir_%s", currentPath, username_array);

    int socket = threadArgs->socket;

    packet_t *initialSyncPacket = create_packet(INITIAL_SYNC, username_array, strlen(username_array) + 1);

    if (send_packet_to_socket(socket, initialSyncPacket) < 0)
    {
        free(PATH);
        destroy_packet(initialSyncPacket);
        return (void*)-1;
    }

    destroy_packet(initialSyncPacket);

    packet_t *packet_buffer = receive_packet_from_socket(socket);
    if (!packet_buffer || packet_buffer->type != FILE_LIST)
    {
        free(PATH);
        destroy_packet(packet_buffer);
        return (void *)-1;
    }

    char *file_list = strdup(packet_buffer->payload);

    // Criando um array para armazenar os nomes dos arquivos do servidor
    const int MAX_FILES = 100;
    char filenames[MAX_FILES][256];
    int file_count = 0;

    char *filename = strtok(file_list, "|");

    while (filename != NULL && file_count < MAX_FILES) {
        printf("Downloading file: %s\n", filename);

        strcpy(filenames[file_count], filename);
        file_count++;

        download_file(filename, socket, 1, threadArgs->username);
        filename = strtok(NULL, "|");
    }
    destroy_packet(packet_buffer);

    // Criando um array para armazenar os nomes dos arquivos do cliente
    char client_filenames[MAX_FILES][256];
    int client_file_count = 0;

    char basepath[100] = CLIENT_FILE_PATH;
    strcat(basepath, threadArgs->username);

    get_client_file_array(basepath, client_filenames, &client_file_count);

    // Imprimir os nomes dos arquivos do servidor
    printf("\nArquivos do Servidor:\n");
    for (int i = 0; i < file_count; ++i) {
        printf("\nServidor - Arquivo %d: %s\n", i + 1, filenames[i]);
    }

    // Imprimir os nomes dos arquivos do cliente
    printf("\nArquivos do Cliente:\n");
    for (int i = 0; i < client_file_count; ++i) {
        printf("\nCliente - Arquivo %d: %s\n", i + 1, client_filenames[i]);
    }

    for (int i = 0; i < client_file_count; ++i) {
        char tempPath[1024];
        int found = 0;
        for (int j = 0; j < file_count; ++j) {
            if (strcmp(client_filenames[i], filenames[j]) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            snprintf(tempPath, sizeof(tempPath), "%s/sync_dir_%s/%s", currentPath, username_array, client_filenames[i]);
            printf("Arquivo no Cliente não encontrado no Servidor: %s\n", client_filenames[i]);
            delete_local_file(client_filenames[i], username_array, currentPath);
        }
    }
    free(file_list);
    free(PATH);
    return NULL;
}

void *start_inotify(void *threadArgsPtr) {    
    #ifdef __linux__
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)threadArgsPtr;
    char username_array[strlen(threadArgs->username) + 1];
    strcpy(username_array, threadArgs->username);

    int socket = threadArgs->socket;

    int inotifyFd, watchFd;

    inotifyFd = inotify_init();
    if (inotifyFd == -1)
    {
        exit(EXIT_FAILURE);
    }

    char *PATH;
    char currentPath[256];

    if (getcwd(currentPath, sizeof(currentPath)) == NULL)
    {
        exit(EXIT_FAILURE);
    }

    size_t pathLength = strlen(currentPath) + strlen("/sync_dir_") + strlen(username_array) + 1;
    PATH = (char *)malloc(pathLength);

    if (PATH == NULL)
    {
        exit(EXIT_FAILURE);
    }

    strcpy(PATH, currentPath);
    strcat(PATH, "/sync_dir_");
    strcat(PATH, username_array);

    watchFd = inotify_add_watch(inotifyFd, PATH, IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE);
    if (watchFd == -1)
    {
        free(PATH);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        handle_inotify_event(inotifyFd, socket, PATH, threadArgs->username);
    }

    close(inotifyFd);
    free(PATH);
#endif
    return NULL;
}

void *watch_server_changes(void *data_arg)
{
    struct ThreadArgs *data = (struct ThreadArgs *)data_arg;
    int notification_socket = createSocket();
    connectToServer(notification_socket, serv_addr);
    packet_t *packet_watch = create_packet(CMD_WATCH_CHANGES, data->username, strlen(data->username) + 1);
    packet_t *packet_buffer = (packet_t*)malloc(sizeof(packet_t));
    if (packet_buffer == NULL)
    {
        return (void *)-1;
    }

    if (send_packet_to_socket(notification_socket, packet_watch) < 0)
    {
        destroy_packet(packet_watch);
        return (void *)-1;
    }

    while (1)
    {
        bzero(packet_buffer, sizeof(packet_t));
        packet_buffer = receive_packet_wo_payload(notification_socket);
        if (receive_packet_payload(notification_socket, packet_buffer) < 0)
        {
            continue;
        }

        if (!packet_buffer)
        {
            continue;
        }

        if (packet_buffer->type == CMD_DELETE)
        {
            char currentPath[256];
            if (getcwd(currentPath, sizeof(currentPath)) == NULL)
            {
                exit(EXIT_FAILURE);
            }
            delete_local_file(packet_buffer->payload, data->username, currentPath);
        }
        else if (packet_buffer->type == CMD_DOWNLOAD)
        {
            download_file(packet_buffer->payload, data_socket, 1, data->username);
        }
    }
    destroy_packet(packet_watch);
    destroy_packet(packet_buffer);
}

int get_sync_dir(char *username, int sockfd)
{
    int result = EXIT_SUCCESS;
    create_folder(username);

    pthread_t initialSyncThread;

    struct ThreadArgs *initialSyncArgs = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    initialSyncArgs->username = strdup(username);
    initialSyncArgs->socket = data_socket;

    if (pthread_create(&initialSyncThread, NULL, handleInitialSync, (void *)initialSyncArgs))
    {
        perror("Erro ao criar a thread get_sync_dir");
        result = EXIT_FAILURE;
    }

    if (pthread_join(initialSyncThread, NULL))
    {
        perror("Erro no wait da thread get_sync_dir");
        result = EXIT_FAILURE;
    }

    struct ThreadArgs *threadArgs = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    threadArgs->username = strdup(username);
    threadArgs->socket = data_socket;

    pthread_t syncThread;
    if (pthread_create(&syncThread, NULL, start_inotify, (void *)threadArgs))
    {
        result = EXIT_FAILURE;
    }
    free((char*)initialSyncArgs->username);
    free(initialSyncArgs);
    free((char*)threadArgs->username);
    free(threadArgs);
    return result;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printUsage();
    }
    char *username = strdup(argv[1]);

    struct hostent *server = getServerHost(argv[2]);

    int port = atoi(argv[3]);
    serv_addr = initializeServerAddress(server, port);

    data_socket = createSocket();

    connectToServer(data_socket, serv_addr);

    char *username_payload = strdup(username);

    if (username_payload == NULL)
    {
        free(username);
        exit(EXIT_FAILURE);
    }

    packet_t *packetUsername = create_packet(CMD_LOGIN, username_payload, strlen(username) + 1);
    free(username_payload);

    if (send_packet_to_socket(data_socket, packetUsername) < 0)
    {
        destroy_packet(packetUsername);
        exit(EXIT_FAILURE);
    }
    destroy_packet(packetUsername);

    if (check_login_response(data_socket) < 0)
    {
        close(data_socket);
        printf("Conexão negada pelo servidor, verifique a quantidade de dispositivos conectados.\n");
        return EXIT_FAILURE;
    }
   
    if (get_sync_dir(username, data_socket) == EXIT_FAILURE)
    {
        free(username);
        exit(EXIT_FAILURE);
    }

    pthread_t server_changes_thread;

    struct ThreadArgs *notification_data = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    notification_data->username = strdup(username);
    notification_data->socket = data_socket;
    
    if (pthread_create(&server_changes_thread, NULL, watch_server_changes, (void *)notification_data))
    {
        free(username);
        free((char*)notification_data->username);
        free(notification_data);
        exit(EXIT_FAILURE);
    }

    pthread_t userInterfaceThread;

    struct ThreadArgs interf_data;
    interf_data.username = strdup(username);
    interf_data.socket = data_socket;

    if (pthread_create(&userInterfaceThread, NULL, userInterface, (void *)&interf_data))
    {
        free(username);
        free((char*)notification_data->username);
        free(notification_data);
        free((char*)interf_data.username);
        exit(EXIT_FAILURE);
    }

    pthread_join(userInterfaceThread, NULL);

    free(username);
    free((char*)notification_data->username);
    free(notification_data);
    free((char*)interf_data.username);

    return 0;
}
