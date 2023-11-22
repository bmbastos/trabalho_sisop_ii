#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#define PORT 4000
#define SERVER_FILE_PATH "./serverFiles/"

typedef struct
{
    char command[50];
    char argument[50];
    int socket;
} thread_data_t;

int send_file(int client_socket, const char *filename)
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s%s", SERVER_FILE_PATH, filename);

    FILE *file = fopen(file_path, "rb");
    if (file == NULL)
    {
        printf("ERROR: Não foi possível abrir o arquivo indicado\n");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0)
    {
        printf("ERROR: Arquivo não existe ou vazio.\n");
        fclose(file);
        return -1;
    }

    send(client_socket, &file_size, sizeof(file_size), 0);

    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
    return 1;
}

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

    if (strcmp(data->command, "download") == 0)
    {
        n = send_file(data->socket, data->argument);

        if (n < 0)
        {
            perror("ERROR writing to socket\n");
            return (void *)-1;
        }

        printf("File %s sent successfully.\n", data->argument);
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
        printf("Ready");
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