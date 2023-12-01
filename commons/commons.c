#include "commons.h"


void destroy_packet(packet_t *packet)
{
    if (packet != NULL)
    {
        free(packet->payload);
        free(packet);
    }
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

int send_packet(int socket, const packet_t *packet)
{
    // Enviar o tipo de pacote
    if (write(socket, &(packet->type), sizeof(type_packet_t)) < 0) {
        perror("Error writing packet type to socket");
        return -1;
    }

    // Enviar o comprimento do payload
    if (write(socket, &(packet->length_payload), sizeof(uint32_t)) < 0) {
        perror("Error writing payload length to socket");
        return -1;
    }

    // Enviar o payload, se houver
    if (packet->length_payload > 0 && packet->payload != NULL) {
        if (write(socket, packet->payload, packet->length_payload) < 0) {
            perror("Error writing payload to socket");
            return -1;
        }
    }

    return 0;
}