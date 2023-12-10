#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/inotify.h>
#endif
#include "../commons/commons.h"
#include "./commands.h"

#define DIR_FOLDER_PREFIX "sync_dir_"

typedef struct
{
    packet_t packet;
    char userpath[50];
    char *username;
    int socket;
} thread_data_t;

typedef struct
{
    char *username;
    int socket;
} notify_data_t;

typedef struct list_of_users
{
    char *username;
    int connections;
    int socket[2];
    int socketNotify[2];
    struct list_of_users *next;
} list_users_t;

list_users_t *users = NULL;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funções relacionadas à lista de usuários
list_users_t *create_new_user(char *user_name, int user_socket);
list_users_t *insert_or_update_new_connection(list_users_t *list, char *username, int user_socket, int *conn_closed, int isNotifySocket);
list_users_t *remove_user_connection(list_users_t *list, char *user_name, int user_socket);
void print_user_list(list_users_t *list);
void free_user_list(list_users_t *list);

// Funções relacionadas ao servidor
void* setup_notification_observer(void* args);
int setupSocket(int *sockfd, int port);
int handle_packet(thread_data_t *data_ptr, int *conn_closed);
packet_t *receive_packet_from_socket(int socket);
void *handle_new_client_connection(void *args);

// ===========================================================================================================================================================
/*
 * FUNÇÕES RELACIONADAS À LISTA DE USUÁRIOS
 */

list_users_t *create_new_user(char *user_name, int user_socket)
{
    list_users_t *new_user = (list_users_t *)calloc(1, sizeof(list_users_t));
    if (new_user == NULL)
    {
        perror("ERROR creating user\n");
        exit(EXIT_FAILURE);
    }

    new_user->username = strdup(user_name);
    if (new_user->username == NULL)
    {
        perror("ERROR duplicating username\n");
        free(new_user);
        exit(EXIT_FAILURE);
    }

    new_user->connections = 1;
    new_user->socket[0] = user_socket;
    new_user->socket[1] = 0;
    new_user->next = NULL;

    return new_user;
}

int send_connection_response(int response, int socket)
{
    char response_str[12];
    snprintf(response_str, sizeof(response_str), "%d", response);

    const packet_t* login_response_packet = create_packet(DATA, response_str, strlen(response_str)+1);
    if (!login_response_packet) {
        return ERROR;
    }

    return send_packet_to_socket(socket, login_response_packet);
}

list_users_t *insert_or_update_new_connection(list_users_t *list, char *username, int user_socket, int *conn_closed, int isNotifySocket)
{
    if (list == NULL) // Lista está vazia: cria um novo usuário
    {
        send_connection_response(EXIT_SUCCESS, user_socket);
        return create_new_user(username, user_socket);
    }
    else if (strcmp(username, list->username) == 0) // A cabeça da lista contém o usuário: verifica se existe espaço para conectar
    {
        if (list->connections < 2)
        {
            if (list->socket[0] == 0)
            {
                list->connections += 1;

                if(isNotifySocket)
                {
                    list->socketNotify[0] = user_socket;
                }
                else
                {
                    list->socket[0] = user_socket;
                    list->socketNotify[0] = 0;
                }
            }
            else
            {
                list->connections += 1;

                if(isNotifySocket)
                {
                    list->socketNotify[1] = user_socket;
                }
                else
                {
                    list->socket[1] = user_socket;
                    list->socketNotify[1] = 0;
                }
            }
        }
        else
        {
            printf("%s atingiu o limite de conexões por usuário.\n", username);
            if ((user_socket) < 0)
            {
                perror("Não foi possível fechar o socket\n");
            }
            send_connection_response(ERROR, user_socket);
            *conn_closed = 1;
        }
        send_connection_response(EXIT_SUCCESS, user_socket);
        return list;
    }
    else // A cabeça da lista não contém o usuário: continua procurando
    {
        list->next = insert_or_update_new_connection(list->next, username, user_socket, conn_closed, isNotifySocket);
        return list;
    }
}

list_users_t *remove_user_connection(list_users_t *list, char *user_name, int user_socket)
{
    printf("Removendo conexão do usuário %s\n", user_name);
    printf("user socket %d\n", user_socket);
    printf("user list %d\n", list->socket[0]);
    printf("user list %d\n", list->socket[1]);

    if (list == NULL)
    {
        printf("Lista de usuários vazia.\n");
        return NULL;
    }

    list_users_t *current = list;
    list_users_t *previous = NULL;

    while (current != NULL)
    {
        if (strcmp(current->username, user_name) == 0)
        {
            printf("Usuário encontrado.\n");
            // Verifica se a conexão existe para o IP e o Socket fornecidos
            for (int i = 0; i < current->connections; ++i)
            {
                if (current->socket[i] == user_socket)
                {
                    printf("Conexão encontrada.\n");
                    // Remove a conexão
                    current->connections -= 1;
                    current->socket[i] = 0;
                    close(user_socket);
                    // Se não houver mais conexões, remova o usuário da lista
                    if (current->connections == 0)
                    {
                        if (previous == NULL)
                        {
                            // O usuário a ser removido é o primeiro da lista
                            list = current->next;
                            free(current->username);
                            free(current);
                            users = list;
                            return list;
                        }
                        else
                        {
                            // O usuário a ser removido não é o primeiro da lista
                            previous->next = current->next;
                            free(current->username);
                            free(current);
                            users = list;
                            return list;
                        }
                    }

                    users = list;
                    return list;
                }
            }
        }

        previous = current;
        current = current->next;
    }

    users = list;

    return list;
}

void print_user_list(list_users_t *list)
{
    if (list == NULL)
    {
        printf("Lista de usuários vazia.\n");
        return;
    }

    printf("Username: %s\n", list->username);
    printf("Connections: %d\n", list->connections);
    printf("Socket 1: %d\n", list->socket[0]);
    printf("Socket 2: %d\n", list->socket[1]);
    printf("\n");

    if (list->next != NULL)
    {
        print_user_list(list->next);
    }
}

void free_user_list(list_users_t *list)
{
    if (list == NULL)
    {
        return;
    }

    free_user_list(list->next);
    free(list->username);
    free(list);
}

// ===========================================================================================================================================================
/*
 * FUNÇÕES RELACIONADAS AO SERVIDOR
 */

int setupSocket(int *sockfd, int port)
{
    struct sockaddr_in serv_addr;

    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd == -1)
    {
        perror("ERROR opening socket");
        return -1;
    }

    if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(serv_addr.sin_zero), 0, 8);

    if (bind(*sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        return -1;
    }

    if (listen(*sockfd, 5) < 0)
    {
        perror("ERROR on listen");
        return -1;
    }

    printf("Server is ready to accept connections on port %d.\n", port);
    return 0;
}

int handle_login(int socket, const char *username)
{
    perror("Login not implemented");
    return ERROR;
}

int handle_packet(thread_data_t *data_ptr, int *conn_closed)
{
    thread_data_t *data = data_ptr;
    int n;
    packet_t packet = data->packet;
    type_packet_t cmd = packet.type;

    switch (cmd)
    {
    case CMD_LOGIN:
        if (handle_login(data->socket, packet.payload) < 0)
        {
            perror("Error handling client login");
            return ERROR;
        }
        break;
    case CMD_UPLOAD:
        if (receive_file(data->socket, data->userpath, packet.payload, packet.length_payload) < 0)
        {
            perror("Error receiving file from socket");
            return ERROR;
        }
        break;
    case CMD_DOWNLOAD:
        if (send_file(data->socket, packet.payload, data->userpath) < 0)
        {
            perror("Error sending file to socket");
            return ERROR;
        }
        break;
    case CMD_DELETE:
        if (delete_file(data->socket, packet.payload, data->userpath) < 0)
        {
            perror("Error deleting file");
            return ERROR;
        }
        break;
    case CMD_LIST_SERVER:
        if (list_server(data->socket, data->userpath) < 0)
        {
            perror("Error on list server");
            return ERROR;
        }
        break;
    case CMD_LIST_CLIENT:
        perror("Unexpected list client command");
        return ERROR;
        break;
    case CMD_EXIT:
        send_connection_response(EXIT_SUCCESS, data->socket);
        *conn_closed = 1;
        pthread_mutex_lock(&list_mutex);
        users = remove_user_connection(users, data->username, data->socket);
        pthread_mutex_unlock(&list_mutex);
        break;
    case DATA:
        printf("DATA packet not expected\n");
        break;
    case CMD_WATCH_CHANGES:
        printf("WATCH_CHANGES packet not expected\n");
        break;
    case CMD_NOTIFY_CHANGES:
        printf("NOTIFY_CHANGES packet not expected\n");
        break;
    case CMD_GET_SYNC_DIR:
        printf("GET_SYNC_DIR packet not expected\n");
        break;
    }
    

    return 0;
}

void get_socket_notify(const char *username, int result[2]) {
    list_users_t *current = users;

    result[0] = -1;
    result[1] = -1;

    while (current != NULL) {
        // Check if the current user is the one we're looking for
        if (strcmp(current->username, username) == 0) {
            result[0] = current->socketNotify[0];
            result[1] = current->socketNotify[1];
            break;
        }
        current = current->next;
    }
}

void send_changes_to_clients(char *username, int inotifyFd, int socket)
{
    #ifdef __linux__
    char buffer[4096];
    ssize_t len;
    char *ptr;
    const struct inotify_event *event;
    char filename[50];
    char filepath[100];
    snprintf(filepath, sizeof(filepath), "./%s%s", DIR_FOLDER_PREFIX, username);

    int userSockets[2];

    while(1)
    {
        len = read(inotifyFd, buffer, sizeof(buffer));

        if (len <= 0)
            continue;

        for (ptr = buffer; ptr < buffer + len; ptr += sizeof(struct inotify_event) + event->len)
        {
            event = (const struct inotify_event *)ptr;
            get_socket_notify(username, userSockets);

            if (event->mask & IN_CREATE)
            {
                strcpy(filename, event->name);
                printf("File %s created.\n", filename);
                
                for(int i = 0; i < 2; ++i)
                {
                    if(userSockets[i] != 0)
                    {
                        packet_t *packet = create_packet(CMD_NOTIFY_CHANGES, filename, strlen(filename)+1);
                        send_packet_to_socket(userSockets[i], packet);
                        print_packet(packet);
                        // send_file(userSockets[i], filename, filepath);
                    }
                }
            }
            else if (event->mask & IN_DELETE)
            {
                strcpy(filename, event->name);
                printf("File %s deleted.\n", filename);
                for(int i = 0; i < 2; ++i)
                {
                    if(userSockets[i] != 0)
                    {
                        packet_t *packet = create_packet(CMD_NOTIFY_CHANGES, filename, strlen(filename)+1);
                        send_packet_to_socket(userSockets[i], packet);
                        print_packet(packet);
                        // delete_file(userSockets[i], filename, filepath);
                    }
                }
            }
            else if (event->mask & IN_MODIFY)
            {
                strcpy(filename, event->name);
                printf("File %s modified.\n", filename);
                for(int i = 0; i < 2; ++i)
                {
                    if(userSockets[i] != 0)
                    {
                        packet_t *packet = create_packet(CMD_NOTIFY_CHANGES, filename, strlen(filename)+1);
                        send_packet_to_socket(userSockets[i], packet);
                        print_packet(packet);
                        // send_file(userSockets[i], filename, filepath);
                    }
                }
            }
        }
    }
    #endif
}

void* setup_notification_observer(void* args)
{
    #ifdef __linux__
    notify_data_t* data = (notify_data_t*)args;

    char path[100] = "./";

    strcat(path, DIR_FOLDER_PREFIX);
    strcat(path, data->username);

    int inotifyFd, watchFd;

    inotifyFd = inotify_init();

    if (inotifyFd == -1)
    {
        perror("inotify_init");
        return (void*)ERROR;
    }

    watchFd = inotify_add_watch(inotifyFd, path, IN_MODIFY | IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE);

    if (watchFd == -1)
    {
        perror("inotify_add_watch");
        return (void*)ERROR;
    }

    printf("Watching for file modifications in the server directory...\n");

    while (1)
    {
        send_changes_to_clients(data->username, inotifyFd, data->socket);
    }
    #endif
    return (void*)0;
}

void update_socket_notify(const char *username, int socket) {
    list_users_t *current = users;

    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            if (!(current->socketNotify[0])) {
                current->socketNotify[0] = socket;
            }
            else if (!(current->socketNotify[1] == 0)) {
                current->socketNotify[1] = socket;
            }
            break;
        }
        current = current->next;
    }
}

void *handle_new_client_connection(void *args)
{
    int socket = *((int *)args);
    free(args);
    char username[50];
    char path[50];
    strcpy(path, DIR_FOLDER_PREFIX);
    int folderChecked = 0;
    packet_t *packet_buffer = malloc(sizeof(packet_t));
    if (packet_buffer == NULL)
    {
        perror("ERROR allocating memory for packet\n");
        return (void *)-1;
    }

    int conn_closed = 0;

    while (1)
    {
        if (conn_closed)
        {
            printf("Conexão do client encerrada.\n");
            break;
        }

        bzero(packet_buffer, sizeof(packet_t));
        packet_buffer = receive_packet_wo_payload(socket);
        if (receive_packet_payload(socket, packet_buffer) < 0)
        {
            perror("Failed to receive packet payload");
            continue;
        }

        if (!packet_buffer)
        {
            printf("Error reading packet from socket. Closing connection\n");
            close(socket);
            break;
        }

        if (packet_buffer->type == CMD_LOGIN)
        {
            strcpy(username, packet_buffer->payload);
            create_folder(username);
            strcat(path, username);
            folderChecked = 1;
            // Se existir a pasta do usuário no servidor, não cria uma nova, simplesmente faz sincronização && cria uma conexão na lista de conexões.
            pthread_mutex_lock(&list_mutex);
            users = insert_or_update_new_connection(users, username, socket, &conn_closed, 0);
            pthread_mutex_unlock(&list_mutex);
            // Se não existir a pasta: cria e faz conexão com o servidor

            if (conn_closed)
            {
                printf("Conexão recusada.\n");
                break;
            }
            continue;
        }

        if (packet_buffer->type == INITIAL_SYNC)
        {   
            strcpy(username, packet_buffer->payload);

            send_files(socket, path);

            if (conn_closed)
            {
                printf("Conexão recusada.\n");
                break;
            }
            continue;
        }

        // if (packet_buffer->type == CMD_WATCH_CHANGES)
        // {
        //     strcpy(username, packet_buffer->payload);
        //     update_socket_notify(username, socket);
        //     pthread_t notify_thread;
        //     notify_data_t* notify_data = malloc(sizeof(notify_data_t));
        //     notify_data->socket = socket;
        //     notify_data->username = packet_buffer->payload;

        //     pthread_create(&notify_thread, NULL, setup_notification_observer, (void*)notify_data);
        //     continue;
        // }

        thread_data_t *thread_data = malloc(sizeof(thread_data_t));
        if (thread_data == NULL)
        {
            perror("ERROR allocating memory for thread data\n");
            continue;
        }

        thread_data->packet = *packet_buffer;
        thread_data->socket = socket;
        thread_data->username = username;
        strcpy(thread_data->userpath, path);

        printf("Received packet:\n");
        print_packet(packet_buffer);

        if (handle_packet(thread_data, &conn_closed) < 0)
        {
            perror("Error handling packet received");
        }
    }

    free(packet_buffer);
    return (void *)0;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, n;
    socklen_t clilen;
    packet_t buffer;
    struct sockaddr_in cli_addr;

    int port = DEFAULT_PORT;

    if (argc > 1)
    {
        int chosen_port = atoi(argv[1]);
        if (chosen_port > 0) 
        {
            port = chosen_port;
        } else {
            printf("Invalid port. Starting server on default port.\n");
        }
    }

    if (setupSocket(&sockfd, port) < 0)
    {
        exit(EXIT_FAILURE);
    }

    clilen = sizeof(struct sockaddr_in);

    while (1)
    {
        newsockfd = -1;
        if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0)
        {
            perror("ERROR on accept\n");
            continue;
        }

        print_socket_info(cli_addr);

        pthread_t thread;
        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = newsockfd;

        if (pthread_create(&thread, NULL, handle_new_client_connection, sock_ptr) < 0)
        {
            perror("ERROR creating thread");
            continue;
        }
    }

    free_user_list(users);
    close(sockfd);

    return 0;
}