#include <stdio.h>
#include <string.h>
#include "./interface.h"
#include "./commands.h"
#include "../commons/commons.h"
#include <unistd.h>

void processInput(char *command, int sockfd)
{
    char arguments[MAX_ARGUMENTS][MAX_ARGUMENT_LENGTH];
    int argCount = 0;

    tokenizeInput(command, arguments, &argCount);

    if (is_equal(command, "upload"))
    {
        if (argCount == 2)
        {
            upload_file(arguments[1], sockfd);
        }
        else
        {
            printf("\nInvalid parameters for 'upload' command.\n");
        }
    }
    else if (is_equal(command, "download"))
    {
        if (argCount == 2)
        {
            download_file(arguments[1], sockfd);
        }
        else
        {
            printf("\nInvalid parameters for 'download' command.\n");
        }
    }
    else if (is_equal(command, "delete"))
    {
        if (argCount == 2)
        {
            delete_file(arguments[1], sockfd);
        }
        else
        {
            printf("\nInvalid parameters for 'delete_file' command.\n");
        }
    }
    else if (is_equal(command, "list_server"))
    {
        if (argCount == 1)
        {
            list_server(sockfd);
        }
        else
        {
            printf("\nInvalid parameters for 'list_server' command.\n");
        }
    }
    else if (is_equal(command, "list_client"))
    {
        if (argCount == 2)
        {
            list_client(sockfd);
        }
        else
        {
            printf("\nInvalid parameters for 'list_client' command.\n");
        }
    }
    else if (is_equal(command, "get_sync_dir"))
    {
        if (argCount == 1)
        {
            get_sync_dir(sockfd);
        }
        else
        {
            printf("\nInvalid parameters for 'get_sync_dir' command.\n");
        }
    }
    else if (is_equal(command, "exit"))
    {
        if (argCount == 1)
        {
            if (close(sockfd) < 0)
            {
                perror("Error closing socket\n");
            } else {
                printf("Socket closed succesfully\n");
            }
        }
        else
        {
            printf("\nInvalid parameters for 'exit' command.\n");
        }
    }
    else
    {
        printf("Invalid command\n");
    }
}

void tokenizeInput(char *command, char arguments[MAX_ARGUMENTS][MAX_ARGUMENT_LENGTH], int *argCount)
{
    char *token = strtok(command, " ");
    while (token != NULL && *argCount < MAX_ARGUMENTS)
    {
        strncpy(arguments[(*argCount)++], token, MAX_ARGUMENT_LENGTH - 1);
        token = strtok(NULL, " ");
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

    if (input[0] != '\0')
    {
        processInput(input, sockfd);
    }
}