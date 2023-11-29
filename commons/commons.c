#include "commons.h"


void destroy_packet(packet_t *packet)
{
    if (packet != NULL)
    {
        free(packet);
    }
}


int is_equal(const char *str1, const char *str2)
{
    if (strcmp(str1, str2) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
