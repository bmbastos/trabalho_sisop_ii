#define PORT 4000
#define SERVER_FILE_PATH "./serverFiles/"
#define SYNC_DIR_BASE_PATH "./sync_dir_"

int send_file(int client_socket, const char *filename, const char *filepath);
int receive_file(int client_socket, const char *filename);
int delete_file(int client_socket, const char *filename, const char *filepath);
int list_client(int socket);
int list_server(int client_socket, const char *userpath);
int get_sync_dir(int client_socket);
int receive_data(int socket, packet_t packet);
