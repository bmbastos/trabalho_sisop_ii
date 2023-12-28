#include "./commands.h"
#include "./interface.h"
#include <sys/select.h>
#include <sys/stat.h>

int upload_file(const char *filepath, int socket) {
    if (filepath == NULL) {
        return ERROR;
    }

    const char *filename = strrchr(filepath, '/');
    if (filename == NULL) {
        return ERROR;
    }
    filename++;
    size_t file_size = get_file_size(filepath);
    char *file_buffer = read_file_into_buffer(filepath);
    if (!file_buffer) {
        return ERROR;
    }

    packet_t *filename_packet = create_packet(CMD_UPLOAD, filename, strlen(filename)+1);
    if (!filename_packet) {
        return ERROR;
    }

    if (send_packet_to_socket(socket, filename_packet) < 0) {
        destroy_packet(filename_packet);
        return ERROR;
    }

    destroy_packet(filename_packet);

    packet_t *data_packet = create_packet(DATA, file_buffer, file_size);

    free(file_buffer);

    if (!data_packet) {
        return ERROR;
    }

    if (send_packet_to_socket(socket, data_packet) < 0) {
        destroy_packet(data_packet);
        return ERROR;
    }
    destroy_packet(data_packet);

    return 0;
}

int download_file(const char *filename, int socket, int on_sync_dir, const char* user)
{
    packet_t *packet = create_packet(CMD_DOWNLOAD, filename, strlen(filename)+1);

    if (send_packet_to_socket(socket, packet))
    {
        return ERROR;
    }

    char file_path[270];
    char basepath[50];

    mkdir(DOWNLOADS_FILE_PATH, 0777);

    if (on_sync_dir)
    {
        strcpy(basepath, CLIENT_FILE_PATH);
        strcat(basepath, user);
    }
        
    else
    {
        strcpy(basepath, DOWNLOADS_FILE_PATH);
    }
        
    snprintf(file_path, sizeof(file_path), "%s/%s", basepath, filename);

    FILE *file_ptr = fopen(file_path, "wb");
    if (file_ptr == NULL)
    {
        return ERROR;
    }

    const packet_t *receivedFilePacket = receive_packet_from_socket(socket);
    if (!receivedFilePacket) {
        fclose(file_ptr);
        return ERROR;
    }

    if (fwrite(receivedFilePacket->payload, 1, receivedFilePacket->length_payload, file_ptr) != receivedFilePacket->length_payload) {
        fclose(file_ptr);
        return ERROR;
    } else {
    }    

    if (fclose(file_ptr) < 0)
    {
        return ERROR;
    }

    return 0;
}

int delete_file(const char *filename, int socket, const char* username)
{
    packet_t *packet = create_packet(CMD_DELETE, filename, strlen(filename)+1);

    if (send_packet_to_socket(socket, packet) < 0)
    {
        return ERROR;
    }

    char currentPath[256];

    if (getcwd(currentPath, sizeof(currentPath)) == NULL)
    {
        exit(EXIT_FAILURE);
    }

    delete_local_file(filename, username, currentPath);

    return 0;
}

void delete_local_file(const char *filename, const char *username, const char *filepath)
{
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "%s/sync_dir_%s/%s", filepath, username, filename);

    if (remove(file_path) != 0)
    {
        return;
    }
}

int list_server(int socket)
{
    packet_t *packet = create_packet(CMD_LIST_SERVER, NULL, 0);

    if (send_packet_to_socket(socket, packet) < 0)
    {
        return ERROR;
    }
    
    const packet_t *packetFileListBuffer = receive_packet_from_socket(socket);

    if (!packetFileListBuffer)
    {
        close(socket);
        return ERROR;
    }

    char *fileList = (char *)malloc(packetFileListBuffer->length_payload + 1);

    strncpy(fileList, packetFileListBuffer->payload, packetFileListBuffer->length_payload);
    
    if(strcmp(fileList, "") == 0) {
        printf("List server is empty!\n");
    }
    else
    {
        printf("(SERVER side debug) file_list server: %s\n", fileList);
    }

    return 0;
}

int list_client(int socket, const char* user)
{
    char file_list[2048] = "";
    char basepath[100] = CLIENT_FILE_PATH;
    strcat(basepath, user);
    get_file_metadata_list(basepath, file_list);

    if(strcmp(file_list, "") == 0) {
        printf("List client is empty!\n");
    }
    else
    {
        printf("(CLIENT side debug) file_list client: %s\n", file_list);
    }

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
            return -1;
        }
        else if (ret == 0)
        {
            return -1;
        }

        received = read(socket, buffer + total_received, length - total_received);

        if (received < 0)
        {
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
        free(packet);
        return ERROR;
    }

    packet_t *packetExitResponse = receive_packet_from_socket(socket);
    int response = ERROR;
    

    if (packetExitResponse) {
        response = atoi(packetExitResponse->payload);
        destroy_packet(packetExitResponse);
    }

    return (response == EXIT_SUCCESS) ? EXIT_SUCCESS : ERROR;
}