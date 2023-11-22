#include <unistd.h>

#define CLIENT_FILE_PATH "./clientFiles/"
#define READ_TIMEOUT 10

void upload_file(const char* filename, int socket);
void download_file(const char* filename, int socket);
void delete_file(const char* filename, int socket);
void list_server(int socket);
void list_client(int socket);
void get_sync_dir(int socket);

int receive_data(int sockfd, void *buffer, size_t length, int timeout_sec);
int send_cmd_to_socket_no_args(int socket, const char *command);
int send_cmd_to_socket(int socket, const char *command, const char *argument);