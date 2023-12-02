#include "commons.h"
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

packet_t* create_packet(type_packet_t type, const char* payload, int payload_length) {
    packet_t* packet = malloc(sizeof(packet_t));
    if (!packet) {
        perror("Could not allocate memory for packet");
        return NULL;
    }

    packet->type = type;
    packet->payload = payload ? clone_string(payload) : NULL;
    packet->length_payload = payload ? strlen(payload) : 0;

    return packet;
}

void destroy_packet(packet_t* packet) {
    if (packet != NULL) {
        if (packet->payload != NULL) {
            free(packet->payload);
            packet->payload = NULL;
        }
        free(packet);
    }
}


const char* get_packet_type_name(type_packet_t type) {
    switch (type) {
        case CMD_LOGIN: return "CMD_LOGIN";
        case DATA: return "DATA";
        case CMD_UPLOAD: return "CMD_UPLOAD";
        case CMD_DOWNLOAD: return "CMD_DOWNLOAD";
        case CMD_DELETE: return "CMD_DELETE";
        case CMD_LIST_SERVER: return "CMD_LIST_SERVER";
        case CMD_LIST_CLIENT: return "CMD_LIST_CLIENT";
        case CMD_GET_SYNC_DIR: return "CMD_GET_SYNC_DIR";
        case CMD_EXIT: return "CMD_EXIT";
        default: return "UNKNOWN";
    }
}

void print_packet(const packet_t* packet) {
    if (!packet) {
        printf("Packet is NULL\n");
        return;
    }
    printf("Packet type: %s (id = %d)\n", get_packet_type_name(packet->type), packet->type);
    printf("Payload length: %u\n", packet->length_payload);
    printf("Payload: ");
    if (packet->payload && packet->length_payload > 0) {
        printf("%s\n", packet->payload);
    } else {
        printf("None\n");
    }
}

int send_packet_to_socket(int socket, packet_t* packet)
{
    size_t total_size = sizeof(packet->type) + sizeof(packet->length_payload) + packet->length_payload;
    
    char *buffer = malloc(total_size);
    if (!buffer) {
        perror("Failed to allocate memory for buffer\n");
        return -1;
    }

    // Copia o type pro buffer
    memcpy(buffer, &(packet->type), sizeof(packet->type));
    size_t bytes_offset = sizeof(packet->type);

    // Copia o length_payload pro buffer
    memcpy(buffer + bytes_offset, &(packet->length_payload), sizeof(packet->length_payload));
    bytes_offset += sizeof(packet->length_payload);

    // Copia o payload pro buffer se existir
    if (packet->payload && packet->length_payload > 0) {
        memcpy(buffer + bytes_offset, packet->payload, packet->length_payload);
    }

    if (sizeof(buffer) <= 0) {
        perror("ERROR! Buffer is empty\n");
        free(buffer);
        return -1;
    }

    if (write(socket, buffer, total_size) < 0) {
        perror("Error writing to socket\n");
        free(buffer);
        return -1;
    }

    print_packet(packet);
    printf("Sent succesfully!\n");

    free(buffer);
    return 0;
}

packet_t* receive_packet_from_socket(int socket)
{
    packet_t *packet = malloc(sizeof(packet_t));
    if (!packet)
    {
        perror("Failed to allocate memory for packet");
        return NULL;
    }

    // Read the type of the packet
    if (read(socket, &(packet->type), sizeof(packet->type)) <= 0)
    {
        perror("Error reading packet type from socket");
        free(packet);
        return NULL;
    }

    // Read the length of the payload
    if (read(socket, &(packet->length_payload), sizeof(packet->length_payload)) <= 0)
    {
        perror("Error reading payload length from socket");
        free(packet);
        return NULL;
    }

    // Read the payload
    packet->payload = NULL;
    if (packet->length_payload > 0)
    {
        packet->payload = malloc(packet->length_payload);
        if (!packet->payload)
        {
            perror("Failed to allocate memory for payload");
            free(packet);
            return NULL;
        }

        if (read(socket, packet->payload, packet->length_payload) <= 0)
        {
            perror("Error reading payload from socket");
            free(packet->payload);
            free(packet);
            return NULL;
        }
    }

    return packet;
}

int is_equal(const char *str1, const char *str2)
{
    if (strcmp(str1, str2) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void print_socket_info(struct sockaddr_in cli_addr) {
    char client_ip[INET_ADDRSTRLEN];

    if (inet_ntop(AF_INET, &cli_addr.sin_addr, client_ip, INET_ADDRSTRLEN) == NULL) {
        perror("inet_ntop failed");
        return;
    }

    int client_port = ntohs(cli_addr.sin_port);

    printf("New connection established. IP: %s, Port: %d\n", client_ip, client_port);
}

char* clone_string(const char* src) {
    char* dest = malloc(strlen(src) + 1);
    if (!dest) {
        fprintf(stderr, "Erro ao alocar memória para a string.\n");
        return NULL;
    }
    strcpy(dest, src);
    return dest;
}

void get_file_metadata_list(const char *basepath, char *file_list)
{
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    dir = opendir(basepath);

    if (dir == NULL)
    {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char file_path[257];
            snprintf(file_path, sizeof(file_path), "%s/%s", basepath, entry->d_name);

            if (stat(file_path, &file_stat) < 0)
            {
                perror("Error getting file stats");
                continue;
            }

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

    closedir(dir);
}

long get_file_size(const char *file_name) {
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror("Error seeking to end of file");
        fclose(file);
        return -1;
    }

    long size = ftell(file);
    if (size == -1) {
        perror("Error getting file size");
        fclose(file);
        return -1;
    }

    fclose(file);
    return size;
}

char* read_file_into_buffer(const char *filename) {
    long size = get_file_size(filename);
    if (size == -1) {
        return NULL;
    }

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    char *buffer = (char *)malloc(size);
    if (buffer == NULL) {
        perror("Error allocating memory for buffer");
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, size, file) != size) {
        perror("Error reading file into buffer");
        free(buffer);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return buffer;
}
