#include "interface.h"


const char* get_filename(const char* filepath) {
    const char* slash = strrchr(filepath, '/');
    if(slash) {
        return slash + 1;
    } else {
        return filepath;
    }
}

int copy_file(const char* src, const char* dest) {
    char buffer[1024];
    size_t bytes;

    FILE *in = fopen(src, "rb");
    if (in == NULL) return -1;

    FILE *out = fopen(dest, "wb");
    if (out == NULL) {
        fclose(in);
        return -1;
    }

    while ((bytes = fread(buffer, 1, sizeof(buffer), in)) != 0) {
        fwrite(buffer, 1, bytes, out);
    }

    fclose(in);
    fclose(out);
    return 0;
}

int copy_to_sync_dir(char* filepath, const char* username) {
    char currentPath[255];

    if (getcwd(currentPath, sizeof(currentPath)) == NULL)
    {
        exit(EXIT_FAILURE);
    }

    const char* filename = get_filename(filepath);
    char newpath[1024];

    snprintf(newpath, sizeof(newpath), "%s/sync_dir_%s/%s", currentPath, username, filename);

    return copy_file(filepath, newpath);
}

int parse_input(char* input, int socket, const char* username) {
    if (strncmp(input, "upload ", 7) == 0) {
        char* argument = input + 7;
        if (upload_file(argument, socket) < 0) {
            perror("Error uploading file");
        }
        copy_to_sync_dir(argument, username);
    } else if (strncmp(input, "download ", 9) == 0) {
        char* argument = input + 9;
        if (download_file(argument, socket, 0, username) < 0) {
            perror("Error downloading file");
        }
    } else if (strncmp(input, "delete ", 7) == 0) {
        char* argument = input + 7;
        if (delete_file(argument, socket, username) < 0) {
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

void printOptionsMenu(void) {
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