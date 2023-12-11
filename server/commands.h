#define DEFAULT_PORT 4000
#define SERVER_FILE_PATH "./serverFiles/"

int send_file(int client_socket, const char *filename, const char *filepath);
int receive_file(int client_socket, const char* user, const char *filename, uint32_t lengthpayload);
int delete_file(int client_socket, const char *filename, const char *filepath);
int list_server(int client_socket, const char *userpath);
void send_files(int socket, const char *userpath);