#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../commons/commons.h"
#include "./commands.h"

typedef struct
{
    char command[50];
    char argument[50];
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

    char cmd[50];
    strcpy(cmd, data->command);

    if (is_equal(cmd, "download"))
    {
        n = send_file(data->socket, data->argument);

        if (n < 0)
        {
            perror("ERROR on download command\n");
            return (void *)-1;
        }

        printf("File %s sent successfully.\n", data->argument);
    }
    else if (is_equal(cmd, "upload"))
    {
        n = receive_file(data->socket, data->argument);

        if (n < 0)
        {
            perror("ERROR on upload command\n");
            return (void *)-1;
        }

        printf("File %s sent successfully.\n", data->argument);
    }
    else if (is_equal(cmd, "delete"))
    {
        n = delete_file(data->socket, data->argument);

        if (n < 0)
        {
            perror("ERROR on delete command\n");
            return (void *)-1;
        }

        printf("File %s deleted successfully.\n", data->argument);
    }
    else if (is_equal(cmd, "list_server"))
    {
        n = list_server(data->socket);

        if (n < 0)
        {
            perror("ERROR on list_server command\n");
            return (void *)-1;
        }

        printf("Server list sent successfully.\n");
    }
    else if (is_equal(cmd, "get_sync_dir"))
    {
        n = get_sync_dir(data->socket);

        if (n < 0)
        {
            perror("ERROR on get_sync_dir command\n");
            return (void *)-1;
        }

        printf("Synchronization successfully.\n");
    }
    else if (is_equal(cmd, "exit"))
    {
        n = close(data->socket);

        if (n < 0)
        {
            perror("ERROR on exit command\n");
            return (void *)-1;
        }

        printf("Exited connection successfully.\n");
    }
    else
    {
        printf("Invalid command received.\n");
    }

    close(data->socket);
    free(data);
    return (void *)0;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, n;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in cli_addr;

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

        bzero(buffer, 256);
        n = read(newsockfd, buffer, 256);

        if (n < 0)
        {
            perror("ERROR reading command from socket\n");
            continue;
        }

        thread_data_t *data = malloc(sizeof(thread_data_t));
        if (data == NULL)
        {
            perror("ERROR allocating memory for thread data");
            continue;
        }

        sscanf(buffer, "%s %s", data->command, data->argument);
        data->socket = newsockfd;

        printf("Received command %s %s\n", data->command, data->argument);

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
    return 0;
}