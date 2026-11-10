#include "string_util.h"

#define MAX_STRING_COPY 256

void string_copy(char *dest, const char *src)
{
    if (dest == NULL || src == NULL)
        return;

    int i = 0;
    while (src[i] != '\0' && i < (MAX_STRING_COPY - 1))
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

int string_equals(const char *str1, const char *str2)
{
    if (str1 == NULL || str2 == NULL)
    {
        return (str1 != str2);
    }

    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0')
    {
        if (str1[i] != str2[i])
        {
            return 1;
        }
        i++;
    }

    return (str1[i] != str2[i]);
}

int string_length(const char *str)
{
    if (str == NULL)
        return 0;

    int len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    return len;
}
