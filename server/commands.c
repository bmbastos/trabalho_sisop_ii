#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
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
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    dir = opendir(userpath);
    if (dir == NULL) {
        perror("Erro ao abrir diretório.");
        return -1;
    }

    char file_list[2048] = "";

    while ((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char file_path[257];
            snprintf(file_path, sizeof(file_path), "%s/%s", userpath, entry->d_name);
            
            stat(file_path, &file_stat);
            
            strcat(file_list, "\n#######################################");
            strcat(file_list, "\nNome: ");
            strcat(file_list, entry->d_name);

            strcat(file_list, "\n\nModification time: ");
            strcat(file_list, ctime(&file_stat.st_mtime));

            strcat(file_list, "\nAccess time: ");
            strcat(file_list, ctime(&file_stat.st_atime));

            strcat(file_list, "\nCreation time: ");
            strcat(file_list, ctime(&file_stat.st_ctime));
            strcat(file_list, "#######################################");
        }
    }

    printf("(server side debug) file_list: %s\n", file_list);
    closedir(dir);
    
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
