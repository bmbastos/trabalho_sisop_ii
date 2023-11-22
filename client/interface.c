#include <stdio.h>
#include <string.h>
#include "./interface.h"
#include <unistd.h>
#define CLIENT_FILE_PATH "./clientFiles/"

#define READ_TIMEOUT 10

void processInput(char *command, int sockfd) {
    char arguments[MAX_ARGUMENTS][MAX_ARGUMENT_LENGTH];
    int argCount = 0;

    tokenizeInput(command, arguments, &argCount);

    if (strcmp(command, "upload") == 0) {
        // Upload function
    } else if (strcmp(command, "download") == 0) {
        if (argCount == 2) {
            handleDownload(arguments[1], sockfd);
        } else {
            printf("\nInvalid parameters for 'download' command.\n");
        }
    } else if (strcmp(command, "delete") == 0) {
        // Delete function
    } else if (strcmp(command, "list_server") == 0) {
        // List server function
    } else if (strcmp(command, "list_client") == 0) {
        // List client function
    } else if (strcmp(command, "get_sync_dir") == 0) {
        // Get sync dir function
    } else if (strcmp(command, "exit") == 0) {
        // Exit function
    } else {
        printf("Invalid command\n");
    }
}

void tokenizeInput(char *command, char arguments[MAX_ARGUMENTS][MAX_ARGUMENT_LENGTH], int *argCount) {
    char *token = strtok(command, " ");
    while (token != NULL && *argCount < MAX_ARGUMENTS) {
        strncpy(arguments[(*argCount)++], token, MAX_ARGUMENT_LENGTH - 1);
        token = strtok(NULL, " ");
    }
}

int receive_data(int sockfd, void *buffer, size_t length, int timeout_sec) {
    fd_set fds;
    struct timeval timeout;
    ssize_t total_received = 0;
    ssize_t received;

    while (total_received < length) {
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        timeout.tv_sec = timeout_sec;
        timeout.tv_usec = 0;

        int ret = select(sockfd + 1, &fds, NULL, NULL, &timeout);
        if (ret < 0) {
            perror("Select error");
            return -1;
        } else if (ret == 0) {
            fprintf(stderr, "Socket timeout\n");
            return -1;
        }

        received = read(sockfd, buffer + total_received, length - total_received);
        if (received < 0) {
            perror("Error reading from socket");
            return -1;
        }

        total_received += received;
    }

    return total_received;
}

void handleDownload(const char *filename, int sockfd) {
    char combined_message[MAX_MESSAGE_LENGTH];
    snprintf(combined_message, sizeof(combined_message), "download %s", filename);
    if (write(sockfd, combined_message, strlen(combined_message)) < 0) {
        perror("Error writing to socket");
        return;
    }

    long file_size;
    if (receive_data(sockfd, &file_size, sizeof(file_size), 10) < 0) {
        fprintf(stderr, "Failed to receive file size\n");
        return;
    }

    char file_path[270];
    snprintf(file_path, sizeof(file_path), "%s%s", CLIENT_FILE_PATH, filename);

    FILE *received_file = fopen(file_path, "wb");
    if (received_file == NULL) {
        perror("Error opening local file");
        return;
    }

    char buffer[BUFFER_SIZE];
    while (file_size > 0) {
        int bytes_received = receive_data(sockfd, buffer, sizeof(buffer), READ_TIMEOUT);
        if (bytes_received < 0) {
            fclose(received_file);
            return;
        }

        fwrite(buffer, 1, bytes_received, received_file);
        file_size -= bytes_received;
    }

    if (fclose(received_file) != 0) {
        perror("Error closing file");
    } else {
        printf("File received from server.\n");
    }
}


void userInterface(int sockfd)
{
	char input[50];

	printf("# upload\n");
	printf("# download\n");
	printf("# delete\n");
	printf("# list_server\n");
	printf("# list_client\n");
	printf("# get_sync_dir\n");
	printf("# exit\n");
	printf("Digite um comando: ");
	fgets(input, sizeof(input), stdin);

	input[strcspn(input, "\n")] = '\0';

	if (input[0] != '\0') {
        processInput(input, sockfd);
    }
}