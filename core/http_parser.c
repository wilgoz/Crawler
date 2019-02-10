#include "../inc/http_parser.h"
#include "../inc/macros.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

/**
 * Month lookup table for date parsing
 */
typedef enum { Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec, err} month;
static month month_lookup(const char *str)
{
    static struct entry {
        month       val;
        const char *key;
    } table[] = {
        { Jan, "Jan" }, { Feb, "Feb" }, { Mar, "Mar" },
        { Apr, "Apr" }, { May, "May" }, { Jun, "Jun" },
        { Jul, "Jul" }, { Aug, "Aug" }, { Sep, "Sep" },
        { Oct, "Oct" }, { Nov, "Nov" }, { Dec, "Dec" },
        { err,  NULL }
    };
    struct entry *p = table;
    for (; p->key != NULL; p++) if (!strcmp(p->key, str)) return p->val;
    return err;
}

/**
 * Parses URLs, enqueues parsed URLs
 */
void parse_links(const char *str, queue_t *q)
{
    char *url;
    const char *begin = str, *end;

    const char *needle_1 = "<a href=\"";
    const char *needle_2 = "\"";
    size_t nlen = strlen(needle_1);

    while ((begin = strstr(begin, needle_1)))
    {
        begin += nlen;
        if ((end = strstr(begin, needle_2)))
        {
            url = malloc(end - begin + 1);
            if (!url) handle_error("malloc parse links");
            memcpy(url, begin, end - begin);
            url[end - begin] = '\0';
            enqueue(q, url);
            free(url);
        }
    }
}

/**
 * Parses "Content-Length: " header from response
 * Returns size
 */
int parse_size(const char *str)
{
    char *copy_str = strdup(str);
    const char *needle = "Content-Length: ";

    size_t ret = 0;
    char *begin = strstr(copy_str, needle);

    if (begin)
    {
        begin += strlen(needle);
        ret = atoi(strtok(begin, "\r\n"));
    }
    else
        ret = strlen(copy_str) + 1;

    free(copy_str);
    return ret;
}

/**
 * Parses & returns HTTP status code
 */
int parse_status(const char *str)
{
    char *copy_url = strdup(str);
    char *token, *token_ptr;

    token = strtok_r(copy_url, " ", &token_ptr);
    token = strtok_r(NULL, " ", &token_ptr);

    int status = atoi(token);
    free(copy_url);
    return status;
}

/**
 * Parses date & time
 * Modifies "res" arg to contain parsed date data
 * Returns time_t datatype, representing seconds since the Epoch
 */
time_t parse_date(const char *str, char **res)
{
    struct tm info;
    time_t ret = 0;

    const char *needle = "Last-Modified: ";
    char *copy_str = strdup(str);
    char *begin = strstr(copy_str, needle);

    if (begin)
    {
        begin += strlen(needle);

        char *datetime = strtok(begin, "\r\n");
        *res = strdup(datetime);

        char *token_ptr, *time_ptr;
        char *token = strtok_r(datetime, " ", &token_ptr);

        // Parses day of the month
        token = strtok_r(NULL, " ", &token_ptr);
        info.tm_mday = atoi(token);

        // Parses month
        token = strtok_r(NULL, " ", &token_ptr);
        info.tm_mon = month_lookup(token);

        // Parses year
        token = strtok_r(NULL, " ", &token_ptr);
        info.tm_year = atoi(token) - 1900;

        // Parses time
        token = strtok_r(NULL, " ", &token_ptr);
        char *time = strdup(token);

        // Parses "Hours" field
        char *tm_tok = strtok_r(time, ":", &time_ptr);
        info.tm_hour = atoi(tm_tok);

        // Parses "Minutes" field
        tm_tok = strtok_r(NULL, ":", &time_ptr);
        info.tm_min = atoi(tm_tok);

        // Parses "Secs" field
        tm_tok = strtok_r(NULL, ":", &time_ptr);
        info.tm_sec = atoi(tm_tok);
        info.tm_isdst = -1;

        ret = mktime(&info);
        free(time);
    }

    free(copy_str);
    return ret;
}

/**
 * Parses any redirections
 * Modifies "res" arg with redirected location
 */
int parse_loc(const char *str, char **res)
{
    char *copy_str = strdup(str);
    const char *needle[2] = {"Location: ", "location: "};
    size_t ret = 0, i = 0;
    for (; i < 2; i++)
    {
        char *begin = strstr(copy_str, needle[i]);
        if (begin)
        {
            ret = 1;
            begin += strlen(needle[i]);
            *res = strdup(strtok(begin, "\r\n"));
            break;
        }
    }
    free(copy_str);
    return ret;
}

/**
 * Parses server connection status
 * Returns 1 if server maintains persistent connection, 0 otherwise
 */
int parse_connection(const char *str)
{
    int ret = 0;
    const char *needle = "Connection: ";

    char *copy_str = strdup(str);
    char *found = strstr(copy_str, needle);

    found += strlen(needle);
    found = strtok(found, "\r\n");

    if (!found) goto end_parse_connection;

    for (char *p = found; *p; p++)
        *p = tolower(*p);
    if (!strcmp(found, "keep-alive"))
        ret = 1;

end_parse_connection:
    free(copy_str);
    return ret;
}
