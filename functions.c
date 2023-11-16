#include <stdio.h>
#include <string.h>
#include "./functions.h"

void processInput(const char* command) {
    if (strcmp(command, "upload") == 0) {
        // Função upload
    } else if (strcmp(command, "download") == 0) {
        // Função download
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

void userInterface() {
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

        processInput(input);
    }
}