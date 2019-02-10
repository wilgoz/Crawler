#ifndef CRAWLER_H
#define CRAWLER_H

#include <time.h>

#define INTERVAL  2
#define MAX_PAGES 10000

struct stats_t
{
    char    *domain;
    int   max_depth;    // User configured max pages
    int       pages;    // Number of 2xx pages

    struct {
        char  *page;
        size_t size;
    } largest;

    struct {
        char *date_time;    // Textual representation of date&time
        char      *page;
        time_t    epoch;    // Seconds since the Epoch
    } recent;

    struct {
        size_t           count; // Indexing
        int   code [MAX_PAGES]; // Status codes 3xx
        char *to   [MAX_PAGES];
        char *from [MAX_PAGES];
    } redirs;

    struct {
        size_t           count;
        int   code [MAX_PAGES];  // 4xx
        char *page [MAX_PAGES];
    } invalids;
};

void            cleanup_stats  (struct stats_t *stats);
struct stats_t *start_crawling (int max, const char *seed);

#endif