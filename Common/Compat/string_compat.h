#pragma once
#include <ctype.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef const char *LPCSTR;
    typedef char *LPSTR;

    inline char *_strlwr(char *str)
    {
        for (int i = 0; str[i] != '\0'; i++)
        {
            str[i] = tolower(str[i]);
        }
        return str;
    }

    inline char *strupr(char *str)
    {
        for (int i = 0; str[i] != '\0'; i++)
        {
            str[i] = toupper(str[i]);
        }
        return str;
    }

    inline void reverse(char s[])
    {
        int i, j;
        char c;

        for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
        {
            c = s[i];
            s[i] = s[j];
            s[j] = c;
        }
    }

    /* itoa:  convert n to characters in s */
    inline void itoa(int n, char s[], int base)
    {
        int i, sign;

        if ((sign = n) < 0) /* record sign */
            n = -n;         /* make n positive */
        i = 0;
        do
        {                          /* generate digits in reverse order */
            s[i++] = n % base + '0'; /* get next digit */
        } while ((n /= base) > 0); /* delete it */
        if (sign < 0)
            s[i++] = '-';
        s[i] = '\0';
        reverse(s);
    }

#define _vsnprintf vsnprintf
#define _snprintf snprintf
#define strlwr _strlwr

#define lstrcat strcat
#define lstrcpy strcpy
#define lstrcpyn strncpy
#define lstrcmpi strcasecmp
#define lstrlen strlen
#define _stricmp strcasecmp
#define stricmp strcasecmp
#define _strnicmp strncasecmp
#define strnicmp strncasecmp
#define _strdup strdup
#define strcmpi strcasecmp

#ifdef __cplusplus
}
#endif