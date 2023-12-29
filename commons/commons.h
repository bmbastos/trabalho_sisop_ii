#ifndef COMMONS_H
#define COMMONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SHOULD_PRINT_PACKETS 0
#define ERROR -1
#define SYNC_DIR_BASE_PATH "./sync_dir_"

typedef enum {
    DATA,
    CMD_LOGIN,
    CMD_UPLOAD,
    CMD_DOWNLOAD,
    CMD_DELETE,
    CMD_LIST_SERVER,
    CMD_LIST_CLIENT,
    CMD_GET_SYNC_DIR,
    CMD_WATCH_CHANGES,
    CMD_NOTIFY_CHANGES,
    CMD_EXIT,
    INITIAL_SYNC,
    FINISH_INITIAL_SYNC,
    FILE_LIST
} type_packet_t;

typedef struct packet {
    type_packet_t type;
    uint32_t length_payload;
    char* payload;
} packet_t;

struct ThreadArgs {
    const char *username;
    int socket;
};

typedef struct thread_data
{    
    struct sockaddr_in serv_addr;
    packet_t packet;
    char *userpath;
    char *username;
    int socket;
} thread_data_t;

// Operações sobre pacotes
packet_t* create_packet(type_packet_t type, const char* payload, int payload_length);
void destroy_packet(packet_t* packet);
const char* get_packet_type_name(type_packet_t type);
void print_packet(const packet_t* packet);
int send_packet_to_socket(int socket, const packet_t* packet);
packet_t * receive_packet_from_socket(int socket);
packet_t* receive_packet_wo_payload(int socket);
int receive_packet_payload(int socket, packet_t *packet);
void destroy_packet(packet_t *packet);

// Operações auxiliares
char* clone_string(const char* src);
int is_equal(const char *str1, const char *str2);
void print_socket_info(struct sockaddr_in cli_addr);
void get_file_metadata_list(const char *basepath, char *file_list);
long get_file_size(const char *filename);
char* read_file_into_buffer(const char *filename);
void create_folder(char username[50]);

#endif