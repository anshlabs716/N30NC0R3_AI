#ifndef WEB_H
#define WEB_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

/* Perform a DuckDuckGo HTML search and return a formatted, human-readable
 * list of the top results (title / URL / snippet). Caller frees the string. */
char* web_search(const char *query);

#ifdef __cplusplus
}
#endif

#endif
