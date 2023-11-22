#include <stdio.h>

#define MAX_ARGUMENTS 5
#define MAX_ARGUMENT_LENGTH 50
#define MAX_INPUT_LENGTH 50
#define MAX_MESSAGE_LENGTH 100
#define BUFFER_SIZE 1024

void userInterface();
void handleDownload(const char *filename, int sockfd);
void tokenizeInput(char *command, char arguments[MAX_ARGUMENTS][MAX_ARGUMENT_LENGTH], int *argCount);
