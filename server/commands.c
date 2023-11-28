#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
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

int receive_file(int client_socket, const char *filename) {
    perror("To be done\n");
    return -1;
}

int delete_file(int client_socket, const char *filename) {
    perror("To be done\n");
    return -1;
}

int list_server(int client_socket) {
    perror("To be done\n");
    return -1;
}

int get_sync_dir(int client_socket) {
    perror("To be done\n");
    return -1;
}
