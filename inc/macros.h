#ifndef MACROS_H
#define MACROS_H

#include <stdio.h>

#define SAFE_FREE(ptr)  \
    do                  \
    {                   \
        if (ptr)        \
        {               \
            free(ptr);  \
            ptr = NULL; \
        }               \
    } while (0)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define ARR_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#endif