#ifndef COMMANDS_H
#define COMMANDS_H
#include <unistd.h>
#include <sys/select.h>
#include "../commons/commons.h"

#define CLIENT_FILE_PATH "./sync_dir"
#define READ_TIMEOUT 10

// COMANDOS PRINCIPAIS
int get_sync_dir(int socket);
int upload_file(const char* filename, int socket);
int download_file(const char* filename, int socket);
int delete_file(const char* filename, int socket);
int list_server(int socket);
int list_client();

// COMANDOS AUXILIARES
int receive_data(int sockfd, void *buffer, size_t length, int timeout_sec);
int send_packet_to_socket(int socket, packet_t* packet);
int send_cmd_to_socket_no_args(int socket, const char *command);
int send_cmd_to_socket(int socket, const char *command, const char *argument);

#endif