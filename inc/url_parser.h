#ifndef URL_PARSER_H
#define URL_PARSER_H

/**
 * Parses simple URL format
 *  - scheme://host:port/path/file
 */

typedef struct url_components {
    char *scheme;
    char   *path;
    char   *port;
    char   *host;
} url_t;

void free_url  (                     url_t *res);
void debug_url (               const url_t *res);
int  parse_url (const char *raw_url, url_t *res);

#endif