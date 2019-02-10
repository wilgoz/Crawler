/*
 * Willy Ghozali
 * 23 April 2018
 */

#include "inc/crawler.h"
#include <stdio.h>

#define TEST_SERVER "https://www.anu.edu.au/"
#define CRAWL_LIMIT 20

static void show_stats(const struct stats_t *stats)
{
    printf("-------------------\n");
    printf("| CRAWLING REPORT |\n");
    printf("-------------------\n\n");
    if (stats->domain)
    {
        printf("CRAWLING DOMAIN\t : %s\n", stats->domain);
    }
    printf("MAX CRAWL LIMIT\t : %d\n\n"     , stats->max_depth);
    printf("CRAWL INTERVALS\t : %d SEC(S)\n", INTERVAL);
    printf("VALID PAGE(S)  \t : %d\n\n"     , stats->pages);
    if (stats->largest.size)
    {
        printf("LARGEST PAGE\t : %s\n"         , stats->largest.page);
        printf("SIZE        \t : %lu BYTES\n\n", stats->largest.size);
    }
    if (stats->recent.page)
    {
        printf("MOST RECENT PAGE : %s\n"  , stats->recent.page);
        printf("MODIFIED ON      : %s\n\n", stats->recent.date_time);
    }
    if (stats->redirs.count)
    {
        printf("%lu REDIRECTED PAGE(S)\n", stats->redirs.count);
        for (size_t i = 0; i < stats->redirs.count; i++)
        {
            printf("\t-> FROM\t : %s\n"  , stats->redirs.from[i]);
            printf("\t   TO  \t : %s\n"  , stats->redirs.to  [i]);
            printf("\t   CODE\t : %d\n\n", stats->redirs.code[i]);
        }
    }
    if (stats->invalids.count)
    {
        printf("%lu INVALID PAGE(S)\n"   , stats->invalids.count);
        for (size_t i = 0; i < stats->invalids.count; i++)
        {
            printf("\t-> PAGE\t : %s\n"  , stats->invalids.page[i]);
            printf("\t   CODE\t : %d\n\n", stats->invalids.code[i]);
        }
    }
}

int main(int argc, char **argv)
{
    char *seed = (argc == 1) ? (char *)TEST_SERVER : argv[1];

    struct stats_t *stats = start_crawling(CRAWL_LIMIT, seed);
    show_stats    (stats);
    cleanup_stats (stats);

    return 0;
}
