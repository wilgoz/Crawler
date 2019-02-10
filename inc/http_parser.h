#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "queue.h"
#include <time.h>

int    parse_status     (const char *str);
int    parse_size       (const char *str);
int    parse_connection (const char *str);
int    parse_loc        (const char *str, char **res);
void   parse_links      (const char *str, queue_t *q);
time_t parse_date       (const char *str, char **res);

#endif