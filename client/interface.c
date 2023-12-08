#include "interface.h"

// Retorna 1 se deve ser encerrada a interface com o servidor
int parse_input(char* input, int socket, const char* username) {
    if (strncmp(input, "upload ", 7) == 0) {
        char* argument = input + 7;
        if (upload_file(argument, socket) < 0) {
            perror("Error uploading file");
        }
    } else if (strncmp(input, "download ", 9) == 0) {
        char* argument = input + 9;
        if (download_file(argument, socket, 0, username) < 0) {
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
    } else if (strcmp(input, "list_client") == 0) {
        if (list_client(socket, username) < 0) {
            perror("Error listing client files");
        }
    } else if (strcmp(input, "exit") == 0) {
        if(close_connection(socket) < 0) {
            perror("Error closing connection.");
            return 0;
        }

        if (close(socket) < 0) {
            perror("Error closing connection.");
            return 0;
        }
        
        printf("\nConexão encerrada com sucesso.\n");

        return 1;
    } else {
        fprintf(stderr, "Tipo de pacote desconhecido ou entrada inválida.\n");
    }

    return 0;
}

void printOptionsMenu() {
    printf("\n");
    printf("# upload\n");
    printf("# download\n");
    printf("# delete\n");
    printf("# list_server\n");
    printf("# list_client\n");
    printf("# exit\n");
    printf("Digite um comando: ");
}

void *userInterface(void *args_ptr)
{
    if (!args_ptr)
    {
        printf(" COULD NOT RECEIVE ARG\n");
    }

    struct ThreadArgs *threadArgs = (struct ThreadArgs *)args_ptr;
    int shouldExit = 0;
    
    while (!shouldExit) {
        char input[50];

        printOptionsMenu();
        fgets(input, sizeof(input), stdin);
        printf("\n");

        input[strcspn(input, "\n")] = '\0';

        if (input[0] != '\0')
        {
            shouldExit = parse_input(input, threadArgs->socket, threadArgs->username);
        }
    }

    return NULL;
}