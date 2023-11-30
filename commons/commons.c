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

int send_packet_to_socket(int socket, packet_t* packet)
{
    size_t total_size = sizeof(packet->type) + sizeof(packet->length_payload) + packet->length_payload;
    
    char *buffer = malloc(total_size);
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
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
        perror("ERROR! Buffer is empty");
        free(buffer);
        return -1;
    }

    if (write(socket, buffer, total_size) < 0) {
        perror("Error writing to socket");
        free(buffer);
        return -1;
    }

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
