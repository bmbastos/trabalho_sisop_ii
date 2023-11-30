#include "./commands.h"
#include "./interface.h"
#include <sys/select.h>

int upload_file(const char *filename, int socket)
{
    packet_t* packet = create_packet(CMD_UPLOAD, filename, strlen(filename));

    if (send_packet_to_socket(socket, packet) < 0)
    {
        perror("Error writing to socket");
        return ERROR;
    }
    perror("Command to be implemented\n");
}

int download_file(const char *filename, int socket)
{
    packet_t* packet = create_packet(CMD_DOWNLOAD, filename, strlen(filename));

    if (send_packet_to_socket(socket, packet))
    {
        perror("Error writing to socket");
        return ERROR;
    }

    long file_size;
    if (receive_data(socket, &file_size, sizeof(file_size), 10) < 0)
    {
        fprintf(stderr, "Failed to receive file size\n");
        return ERROR;
    }

    // char file_path[270];
    // snprintf(file_path, sizeof(file_path), "%s%s", CLIENT_FILE_PATH, filename);

    FILE *received_file = fopen(file_path, "wb");
    if (received_file == NULL)
    {
        perror("Error opening local file");
        return ERROR;
    }

    char buffer[BUFFER_SIZE];
    while (file_size > 0)
    {
        int bytes_received = receive_data(socket, buffer, file_size, READ_TIMEOUT);
        if (bytes_received < 0)
        {
            fclose(received_file);
            return ERROR;
        }

    //     fwrite(buffer, 1, bytes_received, received_file);
    //     file_size -= bytes_received;
    // }

    if (fclose(received_file) != 0)
    {
        perror("Error closing file");
    }
    else
    {
        printf("File received from server.\n");
    }

    return 0;
}

int delete_file(const char *filename, int socket)
{
    packet_t* packet = create_packet(CMD_DELETE, filename, strlen(filename));

    if (send_packet_to_socket(socket, packet) < 0)
    {
        perror("Error writing to socket");
        return ERROR;
    }

    return 0;
}

int list_server(int socket)
{
    packet_t* packet = create_packet(CMD_LIST_SERVER, NULL, 0);

    if (send_packet_to_socket(socket, packet) < 0)
    {
        perror("Error writing to socket");
        return ERROR;
    }

    long file_list_size;
    if (receive_data(socket, &file_list_size, sizeof(file_list_size), 10) < 0)
    {
        fprintf(stderr, "Erro ao receber o tamanho da lista.\n");
        return ERROR;
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
    return 0;
}

int list_client(int socket)
{
    packet_t* packet = create_packet(CMD_LIST_CLIENT, NULL, 0);

    if (send_packet_to_socket(socket, packet) < 0)
    {
        perror("Error writing to socket");
        return ERROR;
    }

    perror("Command to be implemented\n");
    return 0;
}

int get_sync_dir(int socket)
{
    packet_t* packet = create_packet(CMD_GET_SYNC_DIR, NULL, 0);

    if (send_packet_to_socket(socket, packet) < 0)
    {
        perror("Error writing to socket");
        return ERROR;
    }

    perror("Command to be implemented\n");
    return 0;
}

int receive_data(int socket, void *buffer, size_t length, int timeout_sec)
{
    fd_set fds;
    struct timeval timeout;
    ssize_t total_received = 0;
    ssize_t received;

    while (total_received < length)
    {
        FD_ZERO(&fds);
        FD_SET(socket, &fds);

        timeout.tv_sec = timeout_sec;
        timeout.tv_usec = 0;

        int ret = select(socket + 1, &fds, NULL, NULL, &timeout);

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

        received = read(socket, buffer + total_received, length - total_received);

        if (received < 0)
        {
            perror("Error reading from socket");
            return -1;
        }

        total_received += received;
    }

    return total_received;
}
