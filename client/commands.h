#ifndef COMMANDS_H
#define COMMANDS_H
#include <unistd.h>
#include <sys/select.h>
#include "../commons/commons.h"

#define CLIENT_FILE_PATH "./sync_dir_"
#define DOWNLOADS_FILE_PATH "./downloads/"
#define READ_TIMEOUT 10

// COMANDOS PRINCIPAIS
int upload_file(const char* filename, int socket);
int download_file(const char* filename, int socket, int on_sync_dir, const char* user);
int delete_file(const char* filename, int socket, const char* username);
int list_server(int socket);
int list_client(int socket, const char* user);
int close_connection(int socket);

// COMANDOS AUXILIARES
int receive_data(int sockfd, void *buffer, size_t length, int timeout_sec);
void delete_local_file(const char *filename, const char *username, const char *filepath);

#endif