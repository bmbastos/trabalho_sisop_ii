#include <stdio.h>
#include <string.h>
#include "./interface.h"
#include <unistd.h>
#include "./commands.h"

void upload_file(const char* filename, int socket)
{
    if (send_cmd_to_socket(socket, "upload", filename))
    {
        perror("Error writing to socket");
        return;
    }
    perror("Command to be implemented\n");
}

void download_file(const char *filename, int sockfd)
{
    if (send_cmd_to_socket(sockfd, "download", filename))
    {
        perror("Error writing to socket");
        return;
    }

    long file_size;
    if (receive_data(sockfd, &file_size, sizeof(file_size), 10) < 0)
    {
        fprintf(stderr, "Failed to receive file size\n");
        return;
    }

    char file_path[270];
    snprintf(file_path, sizeof(file_path), "%s%s", CLIENT_FILE_PATH, filename);

    FILE *received_file = fopen(file_path, "wb");
    if (received_file == NULL)
    {
        perror("Error opening local file");
        return;
    }

    char buffer[BUFFER_SIZE];
    while (file_size > 0)
    {
        int bytes_received = receive_data(sockfd, buffer, sizeof(buffer), READ_TIMEOUT);
        if (bytes_received < 0)
        {
            fclose(received_file);
            return;
        }

        fwrite(buffer, 1, bytes_received, received_file);
        file_size -= bytes_received;
    }

    if (fclose(received_file) != 0)
    {
        perror("Error closing file");
    }
    else
    {
        printf("File received from server.\n");
    }
}

void delete_file(const char* filename, int socket)
{
    if (send_cmd_to_socket(socket, "delete", filename))
    {
        perror("Error writing to socket");
        return;
    }
    perror("Command to be implemented\n");
}

void list_server(int socket)
{
    if (send_cmd_to_socket_no_args(socket, "list_server"))
    {
        perror("Error writing to socket");
        return;
    }
    perror("Command to be implemented\n");
}

void list_client(int socket)
{
    if (send_cmd_to_socket_no_args(socket, "list_client"))
    {
        perror("Error writing to socket");
        return;
    }
    perror("Command to be implemented\n");
}

void get_sync_dir(int socket)
{
    if (send_cmd_to_socket_no_args(socket, "get_sync_dir"))
    {
        perror("Error writing to socket");
        return;
    }
    perror("Command to be implemented\n");
}

int receive_data(int sockfd, void *buffer, size_t length, int timeout_sec)
{
    fd_set fds;
    struct timeval timeout;
    ssize_t total_received = 0;
    ssize_t received;

    while (total_received < length)
    {
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        timeout.tv_sec = timeout_sec;
        timeout.tv_usec = 0;

        int ret = select(sockfd + 1, &fds, NULL, NULL, &timeout);
        if (ret < 0)
        {
            perror("Select error");
            return -1;
        }
        else if (ret == 0)
        {
            fprintf(stderr, "Socket timeout\n");
            return -1;
        }

        received = read(sockfd, buffer + total_received, length - total_received);
        if (received < 0)
        {
            perror("Error reading from socket");
            return -1;
        }

        total_received += received;
    }

    return total_received;
}

int send_cmd_to_socket(int socket, const char *command, const char *argument)
{
    char combined_message[MAX_MESSAGE_LENGTH];
    snprintf(combined_message, sizeof(combined_message), "%s %s", command, argument);
    if (write(socket, combined_message, strlen(combined_message)) < 0)
    {
        perror("Error writing to socket");
        return -1;
    }
    return 0;
}

int send_cmd_to_socket_no_args(int socket, const char *command)
{
    if (write(socket, command, strlen(command)) < 0)
    {
        perror("Error writing to socket");
        return -1;
    }
    return 0;
}
