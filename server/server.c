#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#define PORT 4000
#define SERVER_FILE_PATH "./serverFiles/"

int send_file(int client_socket, const char* filename) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s%s", SERVER_FILE_PATH, filename);

    FILE* file = fopen(file_path, "rb");
    if (file == NULL) {
        printf("ERROR\n");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        printf("ERROR: Arquivo não existe ou vazio.\n");
        fclose(file);
        return -1;
    }

    send(client_socket, &file_size, sizeof(file_size), 0);

    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
    return 1;
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, n;
	socklen_t clilen;
    char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("ERROR opening socket");
    }
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);     
    
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("ERROR on binding");
    }
	
	listen(sockfd, 5);

    printf("Server is ready to accept connections.");
	
	clilen = sizeof(struct sockaddr_in);
	if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) {
		printf("ERROR on accept");
    }
	
    bzero(buffer, 256);
    n = read(newsockfd, buffer, 256);

	if (n < 0) {
		printf("ERROR reading command from socket");
    }

    char command[50], argument[50];
    sscanf(buffer, "%s %s", command, argument);

	if (strcmp(command, "download") == 0) {
        n = send_file(newsockfd, argument);

        if (n < 0)  {
            printf("ERROR writing to socket");
        }
	}

	close(newsockfd);
	close(sockfd);
	return 0; 
}