#ifndef COMMANDS_H
#define COMMANDS_H
#include <unistd.h>
#include <sys/select.h>
#include "../commons/commons.h"

#define CLIENT_FILE_PATH "./clientFiles/"
#define READ_TIMEOUT 10

// COMANDOS PRINCIPAIS
void get_sync_dir(int socket);
void upload_file(const char* filename, int socket);
void download_file(const char* filename, int socket);
void delete_file(const char* filename, int socket);
void list_server(int socket);
void list_client(int socket);
void exit_connection(int socket);

// COMANDOS AUXILIARES
int receive_data(int sockfd, void *buffer, size_t length, int timeout_sec);
int send_packet_no_args(int socket, const char *command);

#endif