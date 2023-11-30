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

typedef struct
{
    type_packet_t command;
    char argument[50];
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

void *handleInput(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    int n;

    switch(data->command){
        case CMD_DELETE:
            n = delete_file(data->socket, data->argument, data->userpath);

            if (n < 0)
            {
                perror("ERROR on delete command\n");
                return (void *)-1;
            }

            printf("File %s deleted successfully.\n", data->argument);
            break;
        default:
            perror("\nERROR: Command not defined.\n");
            break;
    }

    // if (is_equal(cmd, "download"))
    // {
    //     n = send_file(data->socket, data->argument, data->userpath);

    //     if (n < 0)
    //     {
    //         perror("ERROR on download command\n");
    //         return (void *)-1;
    //     }

    //     printf("File %s sent successfully.\n", data->argument);
    // }
    // else if (is_equal(cmd, "upload"))
    // {
    //     n = receive_file(data->socket, data->argument);

    //     if (n < 0)
    //     {
    //         perror("ERROR on upload command\n");
    //         return (void *)-1;
    //     }

    //     printf("File %s sent successfully.\n", data->argument);
    // }
    // else if (is_equal(cmd, "delete"))
    // {
    //     n = delete_file(data->socket, data->argument, data->userpath);

    //     if (n < 0)
    //     {
    //         perror("ERROR on delete command\n");
    //         return (void *)-1;
    //     }

    //     printf("File %s deleted successfully.\n", data->argument);
    // }
    // else if (is_equal(cmd, "list_server"))
    // {
    //     n = list_server(data->socket, data->userpath);

    //     if (n < 0)
    //     {
    //         perror("ERROR on list_server command\n");
    //         return (void *)-1;
    //     }

    //     printf("Server list sent successfully.\n");
    // }
    // else if (is_equal(cmd, "get_sync_dir"))
    // {
    //     n = get_sync_dir(data->socket);

    //     if (n < 0)
    //     {
    //         perror("ERROR on get_sync_dir command\n");
    //         return (void *)-1;
    //     }

    //     printf("Synchronization successfully.\n");
    // }
    // else if (is_equal(cmd, "exit"))
    // {
    //     n = close(data->socket);

    //     if (n < 0)
    //     {
    //         perror("ERROR on exit command\n");
    //         return (void *)-1;
    //     }

    //     printf("Exited connection successfully.\n");
    // }
    // else
    // {
    //     printf("Invalid command received.\n");

    //     return (void *)0;
    // }

}

int read_packet(int socket, packet_t *buffer) {
    // Ler o tipo do pacote
    if (read(socket, &(buffer->type), sizeof(type_packet_t)) < 0) {
        perror("Error reading packet type from socket");
        return -1;
    }

    // Ler o comprimento do payload
    if (read(socket, &(buffer->length_payload), sizeof(uint32_t)) < 0) {
        perror("Error reading payload length from socket");
        return -1;
    }

    // Alocar memória para o payload
    if (buffer->length_payload > 0) {
        buffer->payload = malloc(buffer->length_payload + 1);
        if (buffer->payload == NULL) {
            perror("Error allocating memory for packet payload");
            return -1;
        }

        // Ler o payload
        if (read(socket, buffer->payload, buffer->length_payload) < 0) {
            perror("Error reading payload from socket");
            free(buffer->payload);
            return -1;
        }
        buffer->payload[buffer->length_payload] = '\0';
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, n;
    socklen_t clilen;
    packet_t buffer;
    struct sockaddr_in cli_addr;
    int folderChecked = 0;

    if (setupSocket(&sockfd) != 0)
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

        while(1) {
            bzero(&buffer, sizeof(packet_t));

            if (read_packet(newsockfd, &buffer) < 0) {
                perror("Error reading packet from socket\n");
                break;
            }

            printf("buffer type is = %d\n", buffer.type);
            printf("buffer payload size is = %d\n", buffer.length_payload);
            printf("buffer payload is = %s\n", buffer.payload);

            char username[1024];
            char user_dir[1050];

            if(folderChecked == 0) {
                strcpy(username, buffer.payload);
                snprintf(user_dir, sizeof(user_dir), "%s%s", SYNC_DIR_BASE_PATH, username);
                if (mkdir(user_dir, 0777) == -1) {
                    printf("Pasta %s já existe.\n", user_dir);
                } else {
                    printf("Pasta %s criada.\n", user_dir);
                }

                folderChecked = 1;
                continue;
            }

            thread_data_t *data = malloc(sizeof(thread_data_t));
            if (data == NULL)
            {
                perror("ERROR allocating memory for thread data");
                continue;
            }

            if (data->argument == NULL)
            {
                perror("Error");
                free(data);
                continue;
            }

            strncpy(data->argument, buffer.payload, buffer.length_payload);

            data->command = buffer.type;
            data->socket = newsockfd;
            strcpy(data->userpath, user_dir);

            printf("\n@ Received command %d %s:", data->command, data->argument);

            pthread_t thread;
            if (pthread_create(&thread, NULL, handleInput, (void *)data) < 0)
            {
                perror("ERROR creating thread");
                free(data);
                continue;
            }

            pthread_detach(thread);
        }
        
        close(sockfd);
    }

    return 0;
}