#include <stdio.h>
#include <string.h>
#include "./interface.h"
#include <unistd.h>
#define CLIENT_FILE_PATH "./clientFiles/"

void processInput(char* command, int sockfd) {
	char arguments[5][50];
	int argCount = 0;

	char *token = strtok((char*)command, " ");
	while (token != NULL) {
		strcpy(arguments[argCount++], token);
		token = strtok(NULL, " ");
	}

	if (strcmp(command, "upload") == 0) {
		// Função upload
	} else if (strcmp(command, "download") == 0) {
		if (argCount == 2) {
			char combined_message[100];
			snprintf(combined_message, sizeof(combined_message), "%s %s", command, arguments[1]);
			write(sockfd, combined_message, strlen(combined_message));

			long file_size;
			read(sockfd, &file_size, sizeof(file_size));
			
			char file_path[270];
			snprintf(file_path, sizeof(file_path), "%s%s", CLIENT_FILE_PATH, arguments[1]);
			
			FILE* received_file = fopen(file_path, "wb");
			if (received_file == NULL) {
					printf("Erro ao criar o arquivo local.\n");
					return;
			}

			char buffer[1024];
			size_t bytes_received;
			while (file_size > 0) {
				bytes_received = read(sockfd, buffer, sizeof(buffer));
				fwrite(buffer, 1, bytes_received, received_file);
				file_size -= bytes_received;
			}
			fclose(received_file);

			printf("Arquivo recebido do servidor.\n");
		} else {
			printf("\nParâmetros inválidos para o commando 'download'.");
		}
	} else if (strcmp(command, "delete") == 0) {
		// Função delete
	} else if (strcmp(command, "list_server") == 0) {
		// Função list_server
	} else if (strcmp(command, "list_client") == 0) {
		// Função list_client
	} else if (strcmp(command, "get_sync_dir") == 0) {
		// Função get_sync_dir
	} else if (strcmp(command, "exit") == 0) {
		// Função exit
	} else {
		printf("Comando inválido\n");
	}
}

void userInterface(int sockfd) {
	char input[50];

	printf("# upload\n");
	printf("# download\n");
	printf("# delete\n");
	printf("# list_server\n");
	printf("# list_client\n");
	printf("# get_sync_dir\n");
	printf("# exit\n");

	while(1) {
		printf("Digite um comando: ");
		fgets(input, sizeof(input), stdin);

		input[strcspn(input, "\n")] = '\0';

		if (input[0] == '\0') {
				continue;
		}

		processInput(input, sockfd);
	}
}