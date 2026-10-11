#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "web.h"

#include <unistd.h>
#include <ctype.h>
#include <glib.h>

struct mem_buffer {
    char  *data;
    size_t size;
};

static size_t curl_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    struct mem_buffer *buf = (struct mem_buffer *)userdata;
    char *grown = (char *)realloc(buf->data, buf->size + total + 1);
    if (!grown) return 0;
    buf->data = grown;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

/* Decode the handful of HTML entities DuckDuckGo emits. */
static void html_unescape(char *s) {
    struct { const char *ent; char ch; } map[] = {
        {"&amp;", '&'}, {"&lt;", '<'}, {"&gt;", '>'},
        {"&quot;", '"'}, {"&#x27;", '\''}, {"&#39;", '\''}, {"&nbsp;", ' '},
        {NULL, 0}
    };
    for (int i = 0; map[i].ent; i++) {
        size_t el = strlen(map[i].ent);
        char *pos;
        while ((pos = strstr(s, map[i].ent)) != NULL) {
            *pos = map[i].ch;
            memmove(pos + 1, pos + el, strlen(pos + el) + 1);
        }
    }
}

/* Strip tags and collapse whitespace in place. */
static void strip_tags(char *s) {
    char *r = s, *w = s;
    int in_tag = 0;
    while (*r) {
        if (*r == '<') { in_tag = 1; r++; continue; }
        if (*r == '>') { in_tag = 0; r++; continue; }
        if (!in_tag) *w++ = *r;
        r++;
    }
    *w = '\0';
    /* collapse whitespace */
    char *o = s;
    int sp = 0;
    for (char *p = s; *p; p++) {
        if (isspace((unsigned char)*p)) {
            if (!sp) { *o++ = ' '; sp = 1; }
        } else {
            *o++ = *p;
            sp = 0;
        }
    }
    *o = '\0';
}

/* Extract the value of an attribute from a tag substring. Returns a malloc'd copy. */
static char* extract_href(const char *html) {
    const char *p = strstr(html, "href=\"");
    if (!p) return NULL;
    p += 6;
    const char *end = strchr(p, '"');
    if (!end) return NULL;
    size_t n = (size_t)(end - p);
    char *url = (char *)calloc(n + 1, sizeof(char));
    memcpy(url, p, n);
    url[n] = '\0';

    /* DuckDuckGo wraps links as //duckduckgo.com/l/?uddg=<encoded>&... */
    char *uddg = strstr(url, "uddg=");
    if (uddg) {
        uddg += 5;
        char *amp = strchr(uddg, '&');
        if (amp) *amp = '\0';
        CURL *curl = curl_easy_init();
        if (curl) {
            char *decoded = curl_easy_unescape(curl, uddg, 0, NULL);
            if (decoded) {
                free(url);
                url = strdup(decoded);
                curl_free(decoded);
            }
            curl_easy_cleanup(curl);
        }
    }
    return url;
}

char* web_search(const char *query) {
    if (!query || !*query) return strdup("No query provided.");

    CURL *curl = curl_easy_init();
    if (!curl) return strdup("Search is unavailable (libcurl failed to start).");

    struct mem_buffer buf = {0};
    buf.data = (char *)calloc(1, 1);
    buf.size = 0;

    char *escaped = curl_easy_escape(curl, query, 0);
    if (!escaped) {
        curl_easy_cleanup(curl);
        free(buf.data);
        return strdup("Search failed while encoding the query.");
    }

    char url[4096];
    snprintf(url, sizeof(url), "https://html.duckduckgo.com/html/?q=%s", escaped);
    curl_free(escaped);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
                     "Mozilla/5.0 (X11; Linux x86_64) neoncore/3.0");

    CURLcode rc = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        free(buf.data);
        return strdup("Search failed to reach DuckDuckGo.");
    }

    /* Parse the top 3 results out of the HTML. */
    size_t cap = 8192;
    char *out = (char *)calloc(cap, sizeof(char));
    snprintf(out, cap, "Top web results for \"%s\":\n", query);

    int found = 0;
    char *cursor = buf.data;
    while (found < 3) {
        char *block = strstr(cursor, "result__body");
        if (!block) block = strstr(cursor, "class=\"result");
        if (!block) break;

        /* look ahead a bounded window for title / url / snippet */
        char window[8192];
        snprintf(window, sizeof(window), "%.8000s", block);
        cursor = block + 12;

        char *title_tag = strstr(window, "result__a");
        char *snippet_tag = strstr(window, "result__snippet");
        if (!title_tag) continue;

        /* title text */
        char *t_open = strchr(title_tag, '>');
        char *t_close = t_open ? strstr(t_open, "</a>") : NULL;
        char title[512] = "";
        if (t_open && t_close && t_close > t_open) {
            size_t n = (size_t)(t_close - (t_open + 1));
            if (n > sizeof(title) - 1) n = sizeof(title) - 1;
            memcpy(title, t_open + 1, n);
            title[n] = '\0';
            html_unescape(title);
            strip_tags(title);
        }

        char *link = extract_href(window);

        char snippet[1024] = "";
        if (snippet_tag) {
            char *s_open = strchr(snippet_tag, '>');
            char *s_close = s_open ? strstr(s_open, "</a>") : NULL;
            if (!s_close) s_close = s_open ? strstr(s_open, "</") : NULL;
            if (s_open && s_close && s_close > s_open) {
                size_t n = (size_t)(s_close - (s_open + 1));
                if (n > sizeof(snippet) - 1) n = sizeof(snippet) - 1;
                memcpy(snippet, s_open + 1, n);
                snippet[n] = '\0';
                html_unescape(snippet);
                strip_tags(snippet);
            }
        }

        if (title[0]) {
            found++;
            char line[2048];
            snprintf(line, sizeof(line), "%d. %s - %s\n   %s\n",
                     found, title, snippet[0] ? snippet : "(no snippet)",
                     link ? link : "(no url)");
            strncat(out, line, cap - strlen(out) - 1);
        }
        free(link);
    }

    free(buf.data);

    if (found == 0) {
        strncat(out, "(No parseable results returned.)\n", cap - strlen(out) - 1);
    }

    /* Be kind to the search endpoint between calls. */
    g_usleep(1500000);
    return out;
}
