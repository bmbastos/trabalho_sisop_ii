#include "interface.h"

int processInput(const char *input, void *sockfd)
{
    // Inicialização
    int ret = 1;
    char arguments[MAX_ARGUMENTS][MAX_ARGUMENT_LENGTH];
    int argCount = 0;

    // Verifica o tipo do pacote, os tokeniza os argumentos (se houver) e os contabiliza.
    type_packet_t type_packet = tokenizeInput(input, arguments, &argCount);

    // Verifica qual é o tipo pacote, constrói o(s) pacote(s) e envia para o servidor.
    // UPLOAD deve ser feito pelo cliente
    // DOWNLOAD deve ser feito pelo servidor
    // DELETE deve ser feito pelo servidor
    // LIST_CLIENT deve ser feito porque?????
    // LIST_SERVER deve ser feito pelo servidor
    // EXIT deve ser feito pelo cliente ou pelo servidor?
    switch (type_packet)
    {
    case DATA:
        packet_t* packet = createPacket(type_packet, (uint32_t) (strlen(input) + 1), input);
        // SEND DATA
        printf("Tipo do pacote: DATA\n");
        printf("Tamanho do pacote: %d\n", packet->length_payload);
        printf("Conteúdo do pacote: %s\n", packet->payload);
        break;
    case CMD_UPLOAD:
        if (argCount == 2)
        {
            packet_t* packet = createPacket(type_packet, (uint32_t) strlen(arguments[1]), arguments[1]);
            // SEND COMMAND UPLOAD
            printf("Tipo do pacote: UPLOAD\n");
            printf("Tamanho do pacote: %d\n", packet->length_payload);
            printf("Conteúdo do pacote: %s\n", packet->payload);
            printf("\n");
            destroy_packet(packet);
            ret = 0;
            break;
        }
        else
        {
            perror("\nERROR: Insuficient number of arguments to UPLOAD.\n");
            break;
        }
    case CMD_DOWNLOAD:
        if (argCount == 2)
        {
            packet_t* packet = createPacket(type_packet, (uint32_t) strlen(arguments[1]), arguments[1]);
            // SEND COMMAND DOWNLOAD
            printf("Tipo do pacote: DOWNLOAD\n");
            printf("Tamanho do pacote: %d\n", packet->length_payload);
            printf("Conteúdo do pacote: %s\n", packet->payload);
            printf("\n");
            destroy_packet(packet);
            ret = 0;
            break;
        }
        else
        {
            perror("\nERROR: Insuficient number of arguments to DOWNLOAD.\n");
            break;
        }
    case CMD_DELETE:
        if (argCount == 2)
        {
            delete_file(arguments[1], *((int *)sockfd));
            break;
        }
        else
        {
            perror("\nERROR: Insuficient number of arguments to DELETE.\n");
            break;
        }
    case CMD_LIST_SERVER:
        if (argCount == 1)
        {
            packet_t* packet = createPacket(type_packet, 0, NULL);
            // SEND COMMAND LIST_SERVER
            printf("Tipo do pacote: LIST_SERVER\n");
            printf("Tamanho do pacote: %d\n", packet->length_payload);
            printf("Conteúdo do pacote: %s\n", packet->payload);
            destroy_packet(packet);
            ret = 0;
            break;
        }
        else
        {
            perror("\nERROR: LIST_SERVER don't expects any argument.\n");
            break;
        }
    case CMD_LIST_CLIENT:
        if (argCount == 1)
        {
            packet_t* packet = createPacket(type_packet, 0, NULL);
            // SEND COMMAND LIST_CLIENT
            printf("Tipo do pacote: LIST_CLIENT\n");
            printf("Tamanho do pacote: %d\n", packet->length_payload);
            printf("Conteúdo do pacote: %s\n", packet->payload);
            destroy_packet(packet);
            ret = 0;
            break;
        }
        else
        {
            perror("\nERROR: LIST_CLIENT don't expects any argument.\n");
            break;
        }
    case CMD_EXIT:
        if (argCount == 1)
        {
            packet_t* packet = createPacket(type_packet, 0, NULL);
            // SEND COMMAND EXIT
            printf("Tipo do pacote: EXIT\n");
            printf("Tamanho do pacote: %d\n", packet->length_payload);
            printf("Conteúdo do pacote: %s\n", packet->payload);
            destroy_packet(packet);
            ret = 0;
            break;
        }
        else
        {
            perror("\nERROR: EXIT don't expects any argument.\n");
            break;
        }
    default:
        perror("\nERROR: Command not defined.\n");
        break;
    }

    return 0;
}

// Cria um pacote simples
packet_t *createPacket(type_packet_t type_packet, uint32_t length_of_payload, const char* payload)
{
    packet_t *packet = (packet_t *)malloc(sizeof(packet_t));

    if (packet == NULL)
    {
        perror("Error creating packet\n");
    }
    else
    {
        packet->type = type_packet;
        packet->length_payload = length_of_payload;

        if(payload != NULL) {
            packet->payload = strdup(payload);
            if (packet->payload == NULL) {
                perror("Error allocating memory for payload\n");
                free(packet);
                free(packet->payload);
                return NULL;
            }
        }
    }
    
    return packet;
}

// Faz a tokenização do input
type_packet_t tokenizeInput(const char *input, char arguments[MAX_ARGUMENTS][MAX_ARGUMENT_LENGTH], int *argCount)
{
    char *input_dup = strdup(input);
    char *token = strtok(input_dup, " ");

    type_packet_t packetType = DATA;

    while (token != NULL && *argCount < MAX_ARGUMENTS)
    {
        strncpy(arguments[(*argCount)++], token, MAX_ARGUMENT_LENGTH - 1);

        // Identificar o tipo de pacote com base no primeiro argumento
        if (*argCount == 1)
        {
            if (strcmp(token, "upload") == 0)
            {
                packetType = CMD_UPLOAD;
            }
            else if (strcmp(token, "download") == 0)
            {
                packetType = CMD_DOWNLOAD;
            }
            else if (strcmp(token, "delete") == 0)
            {
                packetType = CMD_DELETE;
            }
            else if (strcmp(token, "list_server") == 0)
            {
                packetType = CMD_LIST_SERVER;
            }
            else if (strcmp(token, "list_client") == 0)
            {
                packetType = CMD_LIST_CLIENT;
            }
            else if (strcmp(token, "exit") == 0)
            {
                packetType = CMD_EXIT;
            }
        }

        token = strtok(NULL, " ");
    }
    return packetType;
}

void *userInterface(void  *sockfd)
{
    int shouldExit = 0;

    while (!shouldExit) {
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
            shouldExit = processInput(input, sockfd);
        }
    }
}