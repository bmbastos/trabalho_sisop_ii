#ifndef COMMONS_H
#define COMMONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

typedef enum {
    DATA,
    CMD_LOGIN,
    CMD_UPLOAD,
    CMD_DOWNLOAD,
    CMD_DELETE,
    CMD_LIST_SERVER,
    CMD_LIST_CLIENT,
    CMD_EXIT
} type_packet_t;

typedef struct packet {
    type_packet_t type;
    uint32_t length_payload;
    char* payload;
} packet_t;

int is_equal(const char *str1, const char *str2);
void destroy_packet(packet_t *packet);

int send_packet(int socket, const packet_t *packet);

#endif