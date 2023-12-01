#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include "../commons/commons.h"
#include "./commands.h"

#define DIR_FOLDER_PREFIX "sync_dir_"

typedef struct
{
    packet_t packet;
    char userpath[50];
    int socket;
} thread_data_t;

int setupSocket(int *sockfd)
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
    serv_addr.sin_port = htons(PORT);
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

    printf("Server is ready to accept connections.\n");
    return 0;
}

int handle_login(int socket, const char *username)
{
    perror("Login not implemented");
    return ERROR;
}

void *handle_packet(void *data_ptr)
{
    thread_data_t *data = (thread_data_t *)data_ptr;
    int n;
    packet_t packet = data->packet;
    type_packet_t cmd = packet.type;

    switch (cmd)
    {
    case CMD_LOGIN:
        if (handle_login(data->socket, packet.payload) < 0)
        {
            perror("Error handling client login");
            return (void *)ERROR;
        }
    case CMD_UPLOAD:
        if (receive_file(data->socket, packet.payload) < 0)
        {
            perror("Error receiving file from socket");
            return (void *)ERROR;
        }
        break;
    case CMD_DOWNLOAD:
        if (send_file(data->socket, packet.payload, data->userpath) < 0)
        {
            perror("Error sending file to socket");
            return (void *)ERROR;
        }
        break;
    case CMD_DELETE:
        if (delete_file(data->socket, packet.payload, data->userpath) < 0)
        {
            perror("Error deleting file");
            return (void *)ERROR;
        }
        break;
    case CMD_LIST_SERVER:
        if (list_server(data->socket, data->userpath) < 0)
        {
            perror("Error on list server");
            return (void *)ERROR;
        }
        break;
    case CMD_LIST_CLIENT:
        if (list_client(data->socket) < 0)
        {
            perror("Error sending list of clients");
            return (void *)ERROR;
        }
        break;
    case CMD_GET_SYNC_DIR:
        if (get_sync_dir(data->socket) < 0)
        {
            perror("Error sending sync dir");
            return (void *)ERROR;
        }
        break;
    case CMD_EXIT:
        if (close(data->socket) < 0)
        {
            perror("Error closing socket");
            return (void *)ERROR;
        }
        break;
    case DATA:
        if (receive_data(data->socket, packet) < 0)
        {
            perror("Error reciving data from socket");
            return (void *)ERROR;
        }
        break;
    }

    return (void *)0;
}

int receive_packet_from_socket(int socket, packet_t *packet)
{
    if (!packet)
    {
        perror("Packet pointer is NULL");
        return -1;
    }

    // Obtem o tipo do pacote
    if (read(socket, &(packet->type), sizeof(packet->type)) <= 0)
    {
        perror("Error reading packet type from socket");
        return -1;
    }

    // Obtem o tipo do tamanho do payload
    if (read(socket, &(packet->length_payload), sizeof(packet->length_payload)) <= 0)
    {
        perror("Error reading payload length from socket");
        return -1;
    }

    // Obtem o payload
    packet->payload = NULL;
    if (packet->length_payload > 0)
    {
        packet->payload = malloc(packet->length_payload);
        if (!packet->payload)
        {
            perror("Failed to allocate memory for payload");
            return -1;
        }

        if (read(socket, packet->payload, packet->length_payload) <= 0)
        {
            perror("Error reading payload from socket");
            free(packet->payload);
            packet->payload = NULL;
            return -1;
        }
    }

    return 0;
}

void create_folder(char username[50])
{
    char user_dir[100];

    snprintf(user_dir, sizeof(user_dir), "%s%s", SYNC_DIR_BASE_PATH, username);
    if (mkdir(user_dir, 0777) == -1)
    {
        printf("Pasta %s já existe.\n", user_dir);
    }
    else
    {
        printf("Pasta %s criada.\n", user_dir);
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

    while (1)
    {
        bzero(packet_buffer, sizeof(packet_t));

        if (receive_packet_from_socket(socket, packet_buffer) < 0)
        {
            printf("Error reading packet from socket. Closing connection\n");
            close(socket);
            break;
        }

        if (folderChecked == 0)
        {
            if (packet_buffer->type == CMD_LOGIN)
            {
                strcpy(username, packet_buffer->payload);
                create_folder(username);
                strcat(path, username);
                folderChecked = 1;
                continue;
            }
            else
            {
                printf("Expected first packet from client to be a CMD_LOGIN\n");
                continue;
            }
        }

        thread_data_t *thread_data = malloc(sizeof(thread_data_t));
        if (thread_data == NULL)
        {
            perror("ERROR allocating memory for thread data\n");
            continue;
        }

        thread_data->packet = *packet_buffer;
        thread_data->socket = socket;
        strcpy(thread_data->userpath, path);

        printf("Received packet:\n");
        print_packet(packet_buffer);

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_packet, (void *)thread_data) < 0)
        {
            perror("ERROR creating thread");
            continue;
        }

        pthread_detach(thread);
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

    if (setupSocket(&sockfd) < 0)
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

    close(sockfd);

    return 0;
}