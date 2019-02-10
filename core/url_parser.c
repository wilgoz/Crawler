#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include "../inc/url_parser.h"
#include "../inc/macros.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Frees allocated memories after usage & resets
 */
void free_url(url_t *res)
{
    SAFE_FREE(res->scheme);
    SAFE_FREE(res->path);
    SAFE_FREE(res->host);
    SAFE_FREE(res->port);
}

/**
 * Debug parser
 */
void debug_url(const url_t *res)
{
    printf("Scheme\t: %s\nHost\t: %s\nPort\t: %s\nPath\t: %s\n",
           res->scheme, res->host, res->port, res->path);
}

/**
 * Parses & inspects URL
 * Result stored in res arg
 * Returns 0 on invalids, 1 otherwise
 */
static int inspect_url(const char *url)
{
    size_t i = 0;
    const char *invalids[] = {"?", "#", ".pdf", ".doc", ".docx"};
    const char *allowed_schemes[] = {"http://", "https://"};

    for (; i < ARR_SIZE(invalids); i++)
        if (strstr(url, invalids[i]))
            return 0;

    for (i = 0; i < ARR_SIZE(allowed_schemes); i++)
        if (strstr(url, allowed_schemes[i]))
            return 1;
    return 0;
}

int parse_url(const char *raw_url, url_t *res)
{
    // Checks if URL has proper components
    if (!inspect_url(raw_url))
        return 0;

    char *copy_url = strdup(raw_url);
    char *token, *token_ptr;
    char *host_port, *host_port_ptr;

    // Extracts scheme (http, https)
    token = strtok_r(copy_url, ":", &token_ptr);
    res->scheme = strdup(token);

    // Extracts host:port
    token = strtok_r(NULL, "/", &token_ptr);
    host_port = token ? strdup(token) : strdup("");

    // Extracts host
    char *host_tok = strtok_r(host_port, ":", &host_port_ptr);
    if (host_tok)
        res->host = strdup(host_tok);

    // Extracts port (80 if unspecified)
    host_tok = strtok_r(NULL, ":", &host_port_ptr);
    res->port = host_tok ? strdup(host_tok) : strdup("80");

    // Extracts path-to-file
    if (!token_ptr)
        res->path = strdup("/");
    else
        if (asprintf(&res->path, "%s%s", "/", token_ptr) < 0)
            handle_error("asprintf parse_url");

    SAFE_FREE(host_port);
    SAFE_FREE(copy_url);
    return 1;
}
