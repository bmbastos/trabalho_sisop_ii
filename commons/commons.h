#ifndef COMMONS_H
#define COMMONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define ERROR -1

typedef enum {
    DATA,
    CMD_LOGIN,
    CMD_UPLOAD,
    CMD_DOWNLOAD,
    CMD_DELETE,
    CMD_LIST_SERVER,
    CMD_LIST_CLIENT,
    CMD_GET_SYNC_DIR,
    CMD_EXIT
} type_packet_t;

typedef struct packet {
    type_packet_t type;
    uint32_t length_payload;
    char* payload;
} packet_t;

packet_t* create_packet(type_packet_t type, const char* payload, int payload_length);
int send_packet_to_socket(int socket, packet_t* packet);
void destroy_packet(packet_t *packet);
char* clone_string(const char* src);
int is_equal(const char *str1, const char *str2);

int send_packet(int socket, const packet_t *packet);

#endif