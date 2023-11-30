#include "interface.h"

// Retorna 1 se deve ser encerrada a interface com o servidor
int parse_input(char* input, int socket) {
    if (strncmp(input, "upload ", 7) == 0) {
        char* argument = input + 7;
        if (upload_file(argument, socket) < 0) {
            perror("Error uploading file");
        }
    } else if (strncmp(input, "download ", 9) == 0) {
        char* argument = input + 9;
        if (download_file(argument, socket) < 0) {
            perror("Error downloading file");
        }
    } else if (strncmp(input, "delete ", 7) == 0) {
        char* argument = input + 7;
        if (delete_file(argument, socket) < 0) {
            perror("Error deleting file");
        }
    } else if (strcmp(input, "list_server") == 0) {
        if (list_server(socket) < 0) {
            perror("Error listing server files");
        }
    } else if (strcmp(input, "get_sync_dir") == 0) {
        if (get_sync_dir(socket) < 0) {
            perror("Error getting sync dir");
        }
    } else if (strcmp(input, "exit") == 0) {
        if (close(socket) < 0) {
            perror("Error closing socket");
        }
        return 1;
    } else {
        fprintf(stderr, "Tipo de pacote desconhecido ou entrada inválida.\n");
    }

    return 0;
}

void printOptionsMenu() {
    printf("# upload\n");
    printf("# download\n");
    printf("# delete\n");
    printf("# list_server\n");
    printf("# list_client\n");
    printf("# get_sync_dir\n");
    printf("# exit\n");
    printf("Digite um comando: ");
}

void *userInterface(void  *socket_ptr)
{
    int shouldExit = 0;
    int socket = *((int*)socket_ptr);

    while (!shouldExit) {
        char input[50];

        printOptionsMenu();
        fgets(input, sizeof(input), stdin);

        input[strcspn(input, "\n")] = '\0';

        if (input[0] != '\0')
        {
            shouldExit = parse_input(input, socket);
        }
    }
}