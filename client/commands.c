#include "./commands.h"
#include "./interface.h"
#include <sys/select.h>

int upload_file(const char *filename, int socket)
{
    packet_t *packet = create_packet(CMD_UPLOAD, filename, strlen(filename));

    if (send_packet_to_socket(socket, packet) < 0)
    {
        perror("Error writing to socket\n");
        return ERROR;
    }
    perror("Command to be implemented\n");
    return 0;
}

int download_file(const char *filename, int socket)
{
    packet_t *packet = create_packet(CMD_DOWNLOAD, filename, strlen(filename));

    if (send_packet_to_socket(socket, packet))
    {
        perror("Error writing to socket\n");
        return ERROR;
    }

    long file_size;
    if (receive_data(socket, &file_size, sizeof(file_size), 10) < 0)
    {
        fprintf(stderr, "Failed to receive file size\n");
        return ERROR;
    }

    char file_path[270];
    snprintf(file_path, sizeof(file_path), "%s%s", CLIENT_FILE_PATH, filename);

    FILE *file_ptr = fopen(file_path, "wb");
    if (file_ptr == NULL)
    {
        perror("Error opening local file\n");
        return ERROR;
    }

    void *buffer = malloc(file_size);
    int bytes_received = receive_data(socket, buffer, file_size, READ_TIMEOUT);
    if (bytes_received <= 0)
    {
        fclose(file_ptr);
        return ERROR;
    }

    
    int n = fwrite(buffer, file_size, 1, file_ptr);
    if (n < 0)
    {
        perror("Error writing downloaded file locally");
        fclose(file_ptr);
        return ERROR;
    }
    else
    {
        printf("File received successfully from server.\n");
    }

    if (fclose(file_ptr) < 0)
    {
        perror("Error closing file\n");
        return ERROR;
    }

    return 0;
}

int delete_file(const char *filename, int socket)
{
    packet_t *packet = create_packet(CMD_DELETE, filename, strlen(filename));

    if (send_packet_to_socket(socket, packet) < 0)
    {
        perror("Error writing to socket\n");
        return ERROR;
    }

    return 0;
}

int list_server(int socket)
{
    packet_t *packet = create_packet(CMD_LIST_SERVER, NULL, 0);

    if (send_packet_to_socket(socket, packet) < 0)
    {
        perror("Error writing to socket");
        return ERROR;
    }
    
    packet_t *packetFileListBuffer = malloc(sizeof(packet_t));

    if (receive_packet_from_socket(socket, packetFileListBuffer) < 0)
    {
        printf("Error reading packet from socket. Closing connection\n");
        close(socket);
    }

    char *fileList = (char *)malloc(packetFileListBuffer->length_payload + 1);

    strncpy(fileList, packetFileListBuffer->payload, packetFileListBuffer->length_payload);

    printf("%s\n", fileList);
    return 0;
}

int list_client(int socket)
{
    packet_t *packet = create_packet(CMD_LIST_CLIENT, NULL, 0);

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
    packet_t *packet = create_packet(CMD_GET_SYNC_DIR, NULL, 0);

    if (send_packet_to_socket(socket, packet) < 0)
    {
        perror("Error writing to socket\n");
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
            perror("Select error\n");
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
            perror("Error reading from socket\n");
            return -1;
        }

        total_received += received;
    }

    return total_received;
}
