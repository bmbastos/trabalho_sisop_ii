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
void get_sync_dir(const char *username, int data_socket);
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
    const packet_t *login_response_packet = receive_packet_from_socket(socket);
    int response = atoi(login_response_packet->payload);
    if (response == EXIT_SUCCESS)
    {
        printf("Logado com sucesso ao servidor!\n");
        return 0;
    }
    return ERROR;
}

void handle_inotify_event(int fd, int data_socket, char* path, const char *username)
{
#ifdef __linux__
    ssize_t bytesRead;
    char *buffer = NULL;
    size_t bufferSize = 0;
    size_t totalBytesRead = 0;
    size_t chunkSize = 4096;  // Chunk size for reading

    do {
        buffer = realloc(buffer, bufferSize + chunkSize);  // Expand buffer
        if (buffer == NULL) {
            perror("realloc");
            free(buffer);
            exit(EXIT_FAILURE);
        }

        bytesRead = read(fd, buffer + totalBytesRead, chunkSize);
        if (bytesRead == -1) {
            perror("read");
            free(buffer);
            exit(EXIT_FAILURE);
        }

        totalBytesRead += bytesRead;
        bufferSize += chunkSize;

    } while (bytesRead == chunkSize);  // Continue until less data is read than the chunk size

    for (char *ptr = buffer; ptr < buffer + bytesRead;)
    {
        struct inotify_event *event = (struct inotify_event *)ptr;

        char currentPath[1024];
        strcpy(currentPath, path);

        printf("\n[CLIENT - LOG]\tINotify Path: %s\n", currentPath);

        if (event->mask & IN_CLOSE_WRITE)
        {
            printf("\n[CLIENT - LOG]\tArquivo com conteúdo modificado: %s\n", event->name);
            strcat(currentPath, "/");
            strcat(currentPath, event->name);
            upload_file(currentPath, data_socket);
        }

        if (event->mask & IN_CREATE)
        {
            printf("\n[CLIENT - LOG]\tArquivo criado: %s\n", event->name);
            strcat(currentPath, "/");
            strcat(currentPath, event->name);
            upload_file(currentPath, data_socket);
        }
        if (event->mask & IN_MOVED_FROM)
        {
            printf("\n[CLIENT - LOG]\tArquivo retirado da sync_dir: %s\n", event->name);
            delete_file(event->name, data_socket, username);
        }
        if (event->mask & IN_MOVED_TO)
        {
            printf("\n[CLIENT - LOG]\tArquivo movido para sync_dir: %s\n", event->name);
            strcat(currentPath, "/");
            strcat(currentPath, event->name);
            upload_file(currentPath, data_socket);
        }
        if (event->mask & IN_DELETE)
        {
            printf("\n[CLIENT - LOG]\tArquivo deletado: %s\n", event->name);
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
        perror("Error opening directory");
        return;
    }

    *file_count = 0; // Inicializa o contador de arquivos

    // Lê cada arquivo no diretório
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
    printf("\n no handle initial sync\n");
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)threadArgsPtr;
    char username_array[strlen(threadArgs->username) + 1];
    strcpy(username_array, threadArgs->username);

    char *PATH;
    char currentPath[256];

    if (getcwd(currentPath, sizeof(currentPath)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    size_t pathLength = strlen(currentPath) + strlen("/sync_dir_") + strlen(username_array) + 1;
    PATH = (char *)malloc(pathLength);

    if (PATH == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // strcpy(PATH, currentPath);
    // strcat(PATH, "/sync_dir_");
    // strcat(PATH, username_array);

    int socket = threadArgs->socket;

    packet_t *initialSyncPacket = create_packet(INITIAL_SYNC, username_array, strlen(username_array) + 1);

    if (send_packet_to_socket(socket, initialSyncPacket) < 0)
    {
        perror("Failed to send initial sync socket");
    }

    packet_t *packet_buffer = malloc(sizeof(packet_t));
    if (packet_buffer == NULL)
    {
        perror("ERROR allocating memory for packet\n");
        return (void *)-1;
    }

    bzero(packet_buffer, sizeof(packet_t));
    packet_buffer = receive_packet_wo_payload(socket);

    if (receive_packet_payload(socket, packet_buffer) < 0) {
        perror("Failed to receive packet payload");
    }

    if (!packet_buffer || packet_buffer->type != FILE_LIST) {
        printf("Error reading file list from socket or unexpected packet type\n");
    }

    char *file_list = packet_buffer->payload;
    printf("\nthe file list is: %s\n", file_list);

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

    // Criando um array para armazenar os nomes dos arquivos do cliente
    char client_filenames[MAX_FILES][256];
    int client_file_count = 0;

    char basepath[100] = CLIENT_FILE_PATH;
    strcat(basepath, threadArgs->username);

    // Chamando a função para obter os nomes dos arquivos do cliente
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
            printf("\ncurrentPath: %s\n", currentPath);
            printf("Arquivo no Cliente não encontrado no Servidor: %s\n", client_filenames[i]);
            // upload_file(tempPath, socket);
            delete_local_file(client_filenames[i], username_array, currentPath);
        }
    }

    printf("\nfreeing.\n");
    free(packet_buffer);
    return NULL;
}

// void *start_inotify(void *socket_ptr) {
void *start_inotify(void *threadArgsPtr) {    
    #ifdef __linux__
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)threadArgsPtr;
    char username_array[strlen(threadArgs->username) + 1];
    strcpy(username_array, threadArgs->username);

    int socket = threadArgs->socket;

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
    printf("\n\nConcatenated Path: %s\n\n", PATH);

    // Add a watch for the directory
    // watchFd = inotify_add_watch(inotifyFd, PATH, IN_MODIFY | IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE);
    watchFd = inotify_add_watch(inotifyFd, PATH, IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE);
    if (watchFd == -1)
    {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
    }

    printf("Watching for file modifications in the directory...\n");

    // Main event loop
    while (1)
    {
        handle_inotify_event(inotifyFd, socket, PATH, threadArgs->username);
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
    struct ThreadArgs *data = (struct ThreadArgs *)data_arg;
    int notification_socket = createSocket();
    connectToServer(notification_socket, serv_addr);
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
        printf("READY TO RECEIVE MORE NOTIFICATIONS\n\n");
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

        if (packet_buffer->type == CMD_DELETE)
        {
            printf("RECEIVED DELETE NOTIFICATION. UPDATING\n\n");
            printf("FILENAME: %s\n\n", packet_buffer->payload);
            char currentPath[256];
            if (getcwd(currentPath, sizeof(currentPath)) == NULL)
            {
                perror("getcwd");
                exit(EXIT_FAILURE);
            }
            delete_local_file(packet_buffer->payload, data->username, currentPath);
        }
        else if (packet_buffer->type == CMD_DOWNLOAD)
        {
            printf("RECEIVED DOWNLOAD NOTIFICATION. UPDATING\n\n");
            printf("FILENAME: %s\n\n", packet_buffer->payload);

            download_file(packet_buffer->payload, data_socket, 1, data->username);
        }
    }
}

void get_sync_dir(const char *username, int sockfd) {
    char username_array[strlen(username) + 1];
    strcpy(username_array, username);

    create_folder(username_array);

    pthread_t initialSyncThread;

    struct ThreadArgs *initialSyncArgs = malloc(sizeof(struct ThreadArgs));
    initialSyncArgs->username = username;
    initialSyncArgs->socket = data_socket;

    printf("\n\tINICIANDO SINCRONIZAÇÃO - SYNC_DIR\n");

    if (pthread_create(&initialSyncThread, NULL, handleInitialSync, (void *)initialSyncArgs))
    {
        fprintf(stderr, "Erro ao criar thread sync.\n");
        free(initialSyncArgs);
        exit(EXIT_FAILURE);
    }

    printf("\nestamos de volta\n");

    if (pthread_join(initialSyncThread, NULL))
    {
        fprintf(stderr, "Erro ao criar thread de sync.\n");
        free(initialSyncArgs);
        exit(EXIT_FAILURE);
    }

    printf("\n\tFINALIZADA A SINCRONIZAÇÃO - SYNC_DIR\n");

    struct ThreadArgs *threadArgs = malloc(sizeof(struct ThreadArgs));
    threadArgs->username = username;
    threadArgs->socket = data_socket;

    pthread_t syncThread;
    if (pthread_create(&syncThread, NULL, start_inotify, (void *)threadArgs))
    {
        fprintf(stderr, "Erro ao criar thread start_inotify.\n");
        free(threadArgs);
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
    serv_addr = initializeServerAddress(server, port);

    data_socket = createSocket();

    connectToServer(data_socket, serv_addr);

    char *username_payload = strdup(username);

    if (username_payload == NULL)
    {
        perror("Error ao criar username payload.");
        exit(EXIT_FAILURE);
    }

    packet_t *packetUsername = create_packet(CMD_LOGIN, username_payload, strlen(username) + 1);

    if (send_packet_to_socket(data_socket, packetUsername) < 0)
    {
        perror("Error ao enviar username para o servidor.");
        free(username_payload);
        destroy_packet(packetUsername);
        exit(EXIT_FAILURE);
    }

    if (check_login_response(data_socket) < 0)
    {
        close(data_socket);
        printf("Conexão negada pelo servidor\n");
        return EXIT_FAILURE;
    }
    // pthread_t syncThread;
    get_sync_dir(username, data_socket);

    pthread_t server_changes_thread;

    struct ThreadArgs *notification_data = malloc(sizeof(struct ThreadArgs));
    notification_data->socket = data_socket;
    notification_data->username = strdup(username);
    
    if (pthread_create(&server_changes_thread, NULL, watch_server_changes, (void *)notification_data))
    {
        fprintf(stderr, "Erro ao criar thread userInterface.\n");
        free(username_payload);
        destroy_packet(packetUsername);
        exit(EXIT_FAILURE);
    }

    pthread_t userInterfaceThread;

    struct ThreadArgs interf_data;
    interf_data.username = strdup(username);
    interf_data.socket = data_socket;

    if (pthread_create(&userInterfaceThread, NULL, userInterface, (void *)&interf_data))
    {
        fprintf(stderr, "Erro ao criar thread userInterface.\n");
        // free(username_payload);
        destroy_packet(packetUsername);
        exit(EXIT_FAILURE);
    }

    pthread_join(userInterfaceThread, NULL);
    // pthread_join(syncThread, NULL);
    
    // close(data_socket);

    free(username_payload);
    // free(notification_data);
    destroy_packet(packetUsername);

    return 0;
}
