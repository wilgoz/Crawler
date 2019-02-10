#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include "../inc/crawler.h"
#include "../inc/queue.h"
#include "../inc/url_parser.h"
#include "../inc/macros.h"
#include "../inc/http_parser.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>

/**
 * Lazy global vars
 */
typedef struct seen_struct {
    size_t              ix;
    char *links[MAX_PAGES];
} seen_t;

typedef struct crawler {
    int     nodes;   // Current number of pages crawled
    int   sock_fd;   // Current socket
    char    *task;   // Current task (URL to crawl)
    seen_t   seen;   // List of seen links
    int    status;   // Current status code
    url_t     url;   // Current URL components (host, port, path, etc.)
} crawler_t;

static queue_t        *url_q = NULL;    // Discovered URLs queue
static struct stats_t *stats = NULL;    // Overall crawling stats
static crawler_t    *crawler = NULL;    // Current crawling stats

/**
 * Checks for url duplicates
 * Returns 1 if a duplicate is found
 * Slow! Can't be bothered to implement a bloom filter
 */
static int has_seen(const char *str)
{
    for (size_t i = 0; i < crawler->seen.ix; i++)
        if (!strcmp(str, crawler->seen.links[i]))
            return 1;
    return 0;
}

/**
 * Performs DNS mapping with current host & port
 * Returns NULL on failure, list of servers on success
 */
static struct addrinfo *map_dns()
{
    struct addrinfo hints, *server = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(crawler->url.host,
                          crawler->url.port,
                          &hints,
                          &server);
    if (err != 0)
        fprintf(stderr, "getaddrinfo: %s <%s>\n\n",
                gai_strerror(err), crawler->url.host);
    return server;
}

/**
 * Resolves IPv4 from addrinfo
 * Returns IPv4 on success, NULL on failure
 */
static char *resolve_ip()
{
    char *ret = NULL;
    struct addrinfo *server = map_dns();
    if (!server)
        return NULL;
    for (struct addrinfo *p = server; p; p = p->ai_next)
        if (p->ai_family == AF_INET)
        {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)p->ai_addr;
            ret = inet_ntoa(addr_in->sin_addr);
            break;
        }
    freeaddrinfo(server);
    return ret;
}

static char *to_url(const char *ip_addr)
{
    char *ip_address = NULL;
    if (asprintf(&ip_address, "%s:%s%s",
                  ip_addr,
                  crawler->url.port,
                  crawler->url.path) < 0)
        handle_error("asprintf url");
    return ip_address;
}

/**
 * Establishes connection to server
 * Returns 0 on failure, 1 on success
 */
static int begin_connection()
{
    struct addrinfo *server = map_dns(), *p;
    int ret = 0;

    if (!server) return ret;

    for (p = server; p != NULL; p = p->ai_next)
    {
        crawler->sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (crawler->sock_fd == -1)
            continue;
        if (connect(crawler->sock_fd,
                    p->ai_addr,
                    p->ai_addrlen) != -1)
            goto connected;
        close(crawler->sock_fd);
    }
    if (p == NULL)
        fprintf(stderr, "Failed connecting to <%s>\n\n", crawler->url.host);
done:
    freeaddrinfo(server);
    return ret;
connected:
    ret = 1;
    goto done;
}

/**
 * Parses header data (date, status code, size) & updates stats
 */
static void inspect_header(const char *str)
{
    // Parses page status code
    int status = crawler->status = parse_status(str);
    // 4xx
    if (status >= 400 && status < 500)
    {
        int ix = stats->invalids.count++;
        stats->invalids.page[ix] = strdup(crawler->task);
        stats->invalids.code[ix] = status;
    }
    // 3xx (Enqueues redirected loc)
    else if (status >= 300 && status < 400)
    {
        int ix = stats->redirs.count++;
        stats->redirs.from[ix] = strdup(crawler->task);
        stats->redirs.code[ix] = status;
        parse_loc(str, &stats->redirs.to[ix]);
        enqueue(url_q, stats->redirs.to[ix]);
    }
    // 200
    else if (status == 200) stats->pages++;

    // Parses date modified, updates recent pages stats
    // parse_date():
    //   - Stores datetime to temp_date,
    //   - POSIX time to parsed_epoch
    char *temp_date = NULL;
    time_t parsed_epoch = parse_date(str, &temp_date);
    if (parsed_epoch > stats->recent.epoch)
    {
        SAFE_FREE(stats->recent.date_time);
        SAFE_FREE(stats->recent.page);
        stats->recent.epoch = parsed_epoch;
        stats->recent.page  = strdup(crawler->task);
        stats->recent.date_time = strdup(temp_date);
    }
    SAFE_FREE(temp_date);

    // Parses size, updates stats if parsed size is larger
    size_t size = parse_size(str);
    if (size > stats->largest.size)
    {
        SAFE_FREE(stats->largest.page);
        stats->largest.size = size;
        stats->largest.page = strdup(crawler->task);
    }
}

/**
 * Creates & sends HTTP GET/HEAD persistent request
 */
static int request(const char *type)
{
    char *req = NULL;
    int len = asprintf(
        &req, "%s %s%s%s%s", type,
        crawler->url.path, " HTTP/1.0\r\nHost: ",
        crawler->url.host, "\r\nConnection: keep-alive\r\n\r\n");
    if (!len) handle_error("asprintf request");
    int nbytes = send(crawler->sock_fd, req, len, 0); SAFE_FREE(req);
    return nbytes;
}

/**
 * Receives HTTP1.0 HEAD/GET responses
 * Returns overall response
 */
#define BUF_SIZE 4096
static char *receive(const char *type)
{
    size_t size = BUF_SIZE;
    ssize_t nbytes = 0;
    char buf[BUF_SIZE + 1];

    char *res =  calloc(size + 1, sizeof(res));
    char *end = !strcmp(type, "GET") ? "</html>" : "\r\n\r\n";

    while ((nbytes = recv(crawler->sock_fd,
                          buf,
                          BUF_SIZE, 0)) > 0)
    {
        memset(buf + MIN(nbytes, BUF_SIZE), 0, 1);
        strcat(res, buf);
        if (strstr(buf, end))
            break;
        size += BUF_SIZE;
        res = realloc(res, size + 1);
        if (!res) handle_error("realloc receive");
    }
    return res;
}

/**
 * Incredibly naive algorithm
 * Handles HTTP requests and receives, returns HTTP response
 * Reconnects to server and resends request if no responses were received
 * Possible Causes: Timeout | Peer Shutdown
 * Reconnects to server if server closes persistent connection
 */
static char *req_rec_handler(const char *type)
{
    char *res = NULL;
    while (1)
    {
        request(type);
        res = receive(type);
        if (strcmp(res, "") != 0)
            break;
        SAFE_FREE(res);
        begin_connection();
    }
    if (!parse_connection(res))
        begin_connection();
    return res;
}

/**
 * Destructors
 * cleanup_stats() to be called by whichever module initiates the crawling
 */
static void cleanup()
{
    if (crawler->sock_fd != -1)
        close(crawler->sock_fd);
    if (url_q)
        free_queue(url_q);
    free_url(&crawler->url);
    for (size_t i = 0; i < crawler->seen.ix; i++)
        SAFE_FREE(crawler->seen.links[i]);
    SAFE_FREE(crawler->task);
    SAFE_FREE(crawler);
}

void cleanup_stats(struct stats_t *stats)
{
    SAFE_FREE(stats->largest.page);
    SAFE_FREE(stats->domain);
    SAFE_FREE(stats->recent.page);
    SAFE_FREE(stats->recent.date_time);
    for (size_t i = 0; i < stats->redirs.count; i++)
    {
        SAFE_FREE(stats->redirs.from[i]);
        SAFE_FREE(stats->redirs.to[i]);
    }
    for (size_t i = 0; i < stats->invalids.count; i++)
        SAFE_FREE(stats->invalids.page[i]);
    SAFE_FREE(stats);
}

/**
 * Main crawling function, returns crawled stats
 */
struct stats_t *start_crawling(int max, const char *seed)
{
    stats   = calloc(1, sizeof(*stats));
    crawler = calloc(1, sizeof(*crawler));

    crawler->sock_fd = -1;
    stats->max_depth = (max <= MAX_PAGES) ? max : MAX_PAGES;

    // Returns if seed URL is invalid || connection to server fails
    if (!parse_url(seed, &crawler->url) || !begin_connection())
    {
        cleanup();
        return stats;
    }

    stats->domain = strdup(crawler->url.host);
    url_q         = init_queue();
    crawler->task = strdup(seed);

    char *seed_ip = strdup(resolve_ip());
    char *ip_url  = to_url(seed_ip);
    char *res     = NULL;
    char *ip_addr = NULL;

    // Performs BFS as long as there are URLs to work with
    while (ip_url)
    {
        if (!has_seen(ip_url))
        {
            printf("CRAWLING: %s\n\n", crawler->task);
            crawler->seen.links[crawler->seen.ix++] = strdup(ip_url);

            res = req_rec_handler("HEAD");
            inspect_header(res);
            SAFE_FREE(res);

            // Only request GET on status codes 200
            if (crawler->status == 200)
            {
                res = req_rec_handler("GET");
                parse_links(res, url_q);
                SAFE_FREE(res);
            }

            crawler->nodes++;
            sleep(INTERVAL);
        }
        SAFE_FREE(ip_url);
        // Keeps dequeueing if URL is invalid    ||
        //                     DNS mapping fails ||
        //                     Outside seed addr
        while (crawler->nodes < stats->max_depth && !q_empty(url_q))
        {
            free_url(&crawler->url);
            SAFE_FREE(ip_url);
            SAFE_FREE(crawler->task);

            dequeue(url_q, &crawler->task);

            if (parse_url(crawler->task, &crawler->url)
                && (ip_addr = resolve_ip())
                && !strcmp(ip_addr, seed_ip))
            {
                ip_url = to_url(ip_addr);
                break;
            }
        }
    }
    SAFE_FREE(seed_ip); cleanup(); return stats;
}
