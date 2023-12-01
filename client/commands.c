#include "./commands.h"
#include "./interface.h"
#include <sys/select.h>

void upload_file(const char *filename, int socket)
{
    // if (send_packet(socket, "upload", filename))
    // {
    //     perror("Error writing to socket");
    //     return;
    // }
    perror("Command to be implemented\n");
}

void download_file(const char *filename, int sockfd)
{
    // if (send_packet(sockfd, "download", filename))
    // {
    //     perror("Error writing to socket");
    //     return;
    // }

    // long file_size;
    // if (receive_data(sockfd, &file_size, sizeof(file_size), 10) < 0)
    // {
    //     fprintf(stderr, "Failed to receive file size\n");
    //     return;
    // }

    // char file_path[270];
    // snprintf(file_path, sizeof(file_path), "%s%s", CLIENT_FILE_PATH, filename);

    // FILE *received_file = fopen(file_path, "wb");
    // if (received_file == NULL)
    // {
    //     perror("Error opening local file");
    //     return;
    // }

    // char buffer[BUFFER_SIZE];
    // while (file_size > 0)
    // {
    //     int bytes_received = receive_data(sockfd, buffer, file_size, READ_TIMEOUT);
    //     if (bytes_received < 0)
    //     {
    //         fclose(received_file);
    //         return;
    //     }

    //     fwrite(buffer, 1, bytes_received, received_file);
    //     file_size -= bytes_received;
    // }

    // if (fclose(received_file) != 0)
    // {
    //     perror("Error closing file");
    // }
    // else
    // {
    //     printf("File received from server.\n");
    // }
    perror("Command to be implemented\n");
}

void delete_file(const char *filename, int socket)
{
    packet_t* packet = createPacket(CMD_DELETE, (uint32_t)strlen(filename), filename);

    if (send_packet(socket, packet) < 0)
    {
        perror("Error writing to socket");
        return;
    }
    destroy_packet(packet);
}

void list_server(int socket)
{
    if (send_packet_no_args(socket, "list_server"))
    {
        perror("Error writing to socket");
        return;
    }

    long file_list_size;
    if (receive_data(socket, &file_list_size, sizeof(file_list_size), 10) < 0)
    {
        fprintf(stderr, "Erro ao receber o tamanho da lista.\n");
        return;
    }

    char file_list[2040];

    while (file_list_size > 0) {
        int bytes_received = receive_data(socket, file_list, sizeof(file_list), READ_TIMEOUT);
        if (bytes_received < 0)
        {
            break;
        }

        file_list_size -= bytes_received;
    }

    printf("%s\n", file_list);
}

void list_client(int socket)
{
    if (send_packet_no_args(socket, "list_client"))
    {
        perror("Error writing to socket");
        return;
    }
    perror("Command to be implemented\n");
}

void get_sync_dir(int socket)
{
    if (send_packet_no_args(socket, "get_sync_dir"))
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

int send_packet_no_args(int socket, const char *command)
{
    if (write(socket, command, strlen(command)) < 0)
    {
        perror("Error writing to socket");
        return -1;
    }
    return 0;
}

void exit_connection(int sockfd)
{
    if (close(sockfd) == -1)
    {
        perror("Error closing socket");
    }
    else
    {
        printf("Connection closed successfully.\n");
    }
}