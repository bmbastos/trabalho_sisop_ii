#include <stdio.h>
#include <string.h>

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
