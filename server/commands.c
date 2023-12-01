#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
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

    packet_t *packet = create_packet(CMD_DOWNLOAD, filename, strlen(filename));

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

int receive_file(int client_socket, const char *filename) {
    perror("To be done\n");
    return -1;
}

int delete_file(int client_socket, const char *filename, const char *filepath) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", filepath, filename);

    if (remove(file_path) != 0) {
        perror("Error deleting file");
        return -1;
    }

    printf("File %s deleted successfully.\n", filename);

    return 1;
}

int list_client(int socket) {
    perror("To be implemented");
    return ERROR;
}

int receive_data(int socket, packet_t packet) {
    perror("To be implemented");
    return ERROR;
}

int list_server(int client_socket, const char *userpath) {
    char file_list[2048] = "";
    const char *basepath = userpath;

    get_file_metadata_list(basepath, file_list);
    
    printf("(server side debug) file_list: %s\n", file_list);
    
    packet_t* packetFileList = create_packet(CMD_LIST_SERVER, file_list, strlen(file_list));

    if (send_packet_to_socket(client_socket, packetFileList) < 0) {
        perror("Error ao enviar username para o servidor.");
        destroy_packet(packetFileList);
        return -1;
    }

    return 0;
}

int get_sync_dir(int client_socket) {
    perror("To be done\n");
    return -1;
}
