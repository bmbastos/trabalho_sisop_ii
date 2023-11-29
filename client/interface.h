#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./commands.h"
#include "../commons/commons.h"
#include <unistd.h>

#define MAX_ARGUMENTS 5
#define MAX_ARGUMENT_LENGTH 50
#define MAX_INPUT_LENGTH 50
#define MAX_MESSAGE_LENGTH 100
#define BUFFER_SIZE 1024

void *userInterface(void *sockfd);
int send_command_to_socket(int socket, const char *command, const char *argument);
void handleDownload(const char *filename, int sockfd);

type_packet_t tokenizeInput(const char *input, char arguments[MAX_ARGUMENTS][MAX_ARGUMENT_LENGTH], int *argCount);
int processInput(const char* input, int sockfd);
packet_t *createPacket(type_packet_t type_packet, uint32_t length_of_payload, const char* payload);

#endif