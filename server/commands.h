#define PORT 4000
#define SERVER_FILE_PATH "./serverFiles/"

int send_file(int client_socket, const char *filename);
int receive_file(int client_socket, const char *filename);
int delete_file(int client_socket, const char *filename);
int list_server(int client_socket);
int get_sync_dir(int client_socket);