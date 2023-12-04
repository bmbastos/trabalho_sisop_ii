#ifndef COMMANDS_H
#define COMMANDS_H
#include <unistd.h>
#include <sys/select.h>
#include "../commons/commons.h"

#define CLIENT_FILE_PATH "./sync_dir/"
#define DOWNLOADS_FILE_PATH "./downloads/"
#define READ_TIMEOUT 10

// COMANDOS PRINCIPAIS
int get_sync_dir(int socket);
int upload_file(const char* filename, int socket);
int download_file(const char* filename, int socket, int on_sync_dir);
int delete_file(const char* filename, int socket);
int list_server(int socket);
int list_client(int socket);
int close_connection(int socket);

// COMANDOS AUXILIARES
int receive_data(int sockfd, void *buffer, size_t length, int timeout_sec);

#endif