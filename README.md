# Web Crawler
## Willy Ghozali - 23 April 2018

## Description
Naive crawler that scrapes and records website stats within its seed domain. Uses the HTTP 1.0 protocol with support for persistent connections. For UNIX based systems only (POSIX sockets).

Written for the sole purpose of familiarizing myself with the C language.
By no means an effective/efficient crawler.

Stats to be scraped are:
*   Largest page
*   Most recent page
*   Error pages (4xx)
*   Valid pages (2xx)
*   Redirected pages (3xx)

## Limitations
*   Only capable of parsing absolute URLs
*   Disregards URLs with anchor and query tags
*   Disregards robots.txt
*   Slow iterative dupe checks
*   Requires a lot of heap memory to record stats
*   Single threaded - Only allows crawling one seed at a time
*   Fairly new to C - *at time of writing*

## Running
* Build Options:
    * `CMake`
    * `SCons`
* Execution:
    *  `./crawler` after building to crawl the default server
    * `./crawler <URL>` to crawl a specific server
        * eg: `./crawler https://www.bbc.com`