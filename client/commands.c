#include "./commands.h"
#include "./interface.h"
#include <sys/select.h>

int upload_file(const char *filename, int socket) {
    if (filename == NULL) {
        perror("Filename is NULL\n");
        return ERROR;
    }

    packet_t *filename_packet = create_packet(CMD_UPLOAD, filename, strlen(filename));
    if (!filename_packet) {
        perror("Failed to create filename packet\n");
        return ERROR;
    }

    if (send_packet_to_socket(socket, filename_packet) < 0) {
        perror("Error writing filename to socket\n");
        destroy_packet(filename_packet);
        return ERROR;
    }
    destroy_packet(filename_packet);

    char filepath[60] = CLIENT_FILE_PATH;
    strcat(filepath, filename);
    printf("filepath: %s\n", filepath);

    size_t file_size = get_file_size(filepath);
    char *file_buffer = read_file_into_buffer(filepath);
    if (!file_buffer) {
        perror("Failed to read file into buffer\n");
        return ERROR;
    }

    packet_t *data_packet = create_packet(DATA, file_buffer, file_size);
    if (data_packet->length_payload == 0)
    {
        perror("Packet is malformed\n");
        return ERROR;
    }
    free(file_buffer);

    if (!data_packet) {
        perror("Failed to create data packet\n");
        return ERROR;
    }

    if (send_packet_to_socket(socket, data_packet) < 0) {
        perror("Error writing file to socket\n");
        destroy_packet(data_packet);
        return ERROR;
    }
    destroy_packet(data_packet);

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

    char file_path[270];
    snprintf(file_path, sizeof(file_path), "%s/%s", CLIENT_FILE_PATH, filename);

    FILE *file_ptr = fopen(file_path, "wb");
    if (file_ptr == NULL)
    {
        perror("Error opening local file\n");
        return ERROR;
    }

    const packet_t *receivedFilePacket = receive_packet_from_socket(socket);
    if (!receivedFilePacket) {
        fprintf(stderr, "Failed to receive file packet\n");
        fclose(file_ptr);
        return ERROR;
    }

    if (fwrite(receivedFilePacket->payload, 1, receivedFilePacket->length_payload, file_ptr) != receivedFilePacket->length_payload) {
        perror("Error writing downloaded file locally");
        fclose(file_ptr);
        return ERROR;
    } else {
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
    
    const packet_t *packetFileListBuffer = receive_packet_from_socket(socket);

    if (!packetFileListBuffer)
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
    char file_list[2048] = "";
    const char *basepath = CLIENT_FILE_PATH;

    get_file_metadata_list(basepath, file_list);

    printf("(CLIENT side debug) file_list client: %s\n", file_list);

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

int close_connection(int socket) {
    packet_t *packet = create_packet(CMD_EXIT, NULL, 0);

    if (send_packet_to_socket(socket, packet) < 0)
    {
        perror("Error writing to socket\n");
        return ERROR;
    }

    const packet_t *packetExitResponse = receive_packet_from_socket(socket);
    int response = atoi(packetExitResponse->payload);

    if (response == EXIT_SUCCESS)
    {
        return EXIT_SUCCESS;
    }

    return ERROR;
}