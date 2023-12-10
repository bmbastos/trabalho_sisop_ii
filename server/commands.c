#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include "../commons/commons.h"
#include "./commands.h"

int send_file(int client_socket, const char *filename, const char *filepath)
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", filepath, filename);

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

    packet_t *packet = create_packet(CMD_DOWNLOAD, filename, strlen(filename)+1);

    packet->payload = malloc(file_size);
    packet->length_payload = file_size;

    if (fread(packet->payload, 1, file_size, file) != file_size)
    {
        printf("ERROR: Não foi possível ler o arquivo indicado\n");
        fclose(file);
        return -1;
    }

    if (send_packet_to_socket(client_socket, packet) < 0)
    {
        perror("Error writing to socket\n");
        return ERROR;
    }

    fclose(file);
    printf("%s send successfully\n", filename);

    return 1;
}

int receive_file(int client_socket, const char *user, const char *file_name, uint32_t lengthpayload)
{
    packet_t *data_packet = receive_packet_from_socket(client_socket);
    if (data_packet == NULL || data_packet->type != DATA)
    {
        perror("Failed to receive DATA package.");
        return ERROR;
    }
    // if (data_packet == NULL || data_packet->length_payload == 0 || data_packet->type != DATA)
    // {
    //     perror("Failed to receive DATA package.");
    //     return ERROR;
    // }

    char *filename = (char *)malloc(lengthpayload + 1);
    strncpy(filename, file_name, lengthpayload);
    filename[lengthpayload] = '\0';

    char filepath[100];
    snprintf(filepath, sizeof(filepath), "%s/%s", user, filename);
    printf("path: %s\n", filepath);
    FILE *file = fopen(filepath, "wb");
    if (!file) {
        perror("Failed to open file for writing");
        return ERROR;
    }

    // Write the payload to the file
    size_t b_written = fwrite(data_packet->payload, 1, data_packet->length_payload, file);
    if (b_written != data_packet->length_payload) {
        perror("Failed to write the complete payload to the file");
        fclose(file);
        return ERROR;
    }

    fclose(file);

    printf("File received and saved successfully\n");
    return 0;
}

int delete_file(int client_socket, const char *filename, const char *filepath)
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", filepath, filename);

    if (remove(file_path) != 0)
    {
        perror("Error deleting file");
        return -1;
    }

    printf("File %s deleted successfully.\n", filename);

    return 1;
}

int list_server(int client_socket, const char *userpath)
{
    char file_list[2048] = "";
    const char *basepath = userpath;

    get_file_metadata_list(basepath, file_list);

    printf("(server side debug) file_list: %s\n", file_list);

    packet_t *packetFileList = create_packet(CMD_LIST_SERVER, file_list, strlen(file_list)+1);

    if (send_packet_to_socket(client_socket, packetFileList) < 0)
    {
        perror("Error ao enviar lista de arquivos para o cliente.");
        destroy_packet(packetFileList);
        return -1;
    }

    return 0;
}

void send_files(int socket, const char *userpath)
{
    DIR *dir;
    struct dirent *entry;
    char file_list[2048] = "";
    char delimiter = '|';
    int has_files = 0;

    dir = opendir(userpath);

    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if (has_files) {
                strncat(file_list, &delimiter, 1);
            } else {
                has_files = 1;
            }
            strcat(file_list, entry->d_name);
        }
    }

    closedir(dir);

    send_packet_to_socket(socket, create_packet(FILE_LIST, file_list, strlen(file_list) + 1));
}