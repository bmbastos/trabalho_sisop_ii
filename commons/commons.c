#include "commons.h"


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

const char* get_packet_type_name(type_packet_t type) {
    switch (type) {
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

char* clone_string(const char* src) {
    char* dest = malloc(strlen(src) + 1);
    if (!dest) {
        fprintf(stderr, "Erro ao alocar memória para a string.\n");
        return NULL;
    }
    strcpy(dest, src);
    return dest;
}
