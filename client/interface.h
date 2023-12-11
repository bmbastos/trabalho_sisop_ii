#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./commands.h"
#include "../commons/commons.h"
#include <unistd.h>
#include <errno.h>

#define MAX_ARGUMENTS 5
#define MAX_ARGUMENT_LENGTH 50
#define MAX_INPUT_LENGTH 50
#define MAX_MESSAGE_LENGTH 100
#define BUFFER_SIZE 1024

// Funções principais da interface
void printOptionsMenu(void);
void *userInterface(void *args_ptr);
int parse_input(char* input, int socket, const char* username);

// Funções auxiliares
const char* get_filename(const char* filepath);
int copy_file(const char* src, const char* dest);
int copy_to_sync_dir(char* filepath, const char* username);

#endif