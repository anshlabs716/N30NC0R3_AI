#ifndef BRAIN_H
#define BRAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

/*
 * Intent router for neoncore.
 *
 * The router matches user input against an ordered list of structural
 * (POSIX ERE) patterns. A handful of deterministic local tools (arithmetic,
 * timers, dice, ASCII art, app control) are answered directly; everything
 * conversational is flagged with use_llm so the language model writes the
 * reply (optionally enriched with web-search context). There are no canned
 * answers.
 */
typedef enum {
    INTENT_NONE = 0,
    INTENT_CONVERT,
    INTENT_CALCULATE,
    INTENT_GREETING,
    INTENT_FAREWELL,
    INTENT_NAME_ASK,
    INTENT_NAME_GIVE,
    INTENT_HOW_ARE_YOU,
    INTENT_HELP,
    INTENT_CLEAR,
    INTENT_SAVE,
    INTENT_LOAD,
    INTENT_TIME,
    INTENT_WEATHER,
    INTENT_PASSWORD,
    INTENT_ROLL,
    INTENT_FLIP,
    INTENT_REMINDER,
    INTENT_TODO,
    INTENT_NOTE,
    INTENT_PATTERN,
    INTENT_ASCII,
    INTENT_LYRICS,
    INTENT_POEM,
    INTENT_STORY,
    INTENT_BATTLE,
    INTENT_JOKE,
    INTENT_QUOTE,
    INTENT_FACT,
    INTENT_NEWS,
    INTENT_WIKIPEDIA,
    INTENT_SEARCH,
    INTENT_DEFINE,
    INTENT_WEB_SEARCH,
    INTENT_TERMINAL,
    INTENT_FALLBACK
} IntentType;

typedef struct {
    IntentType type;
    char *text;      /* Local response, or the extracted query payload. */
    char *payload;   /* Extra data (e.g. the query to search / define).   */
    int   use_llm;   /* 1 => route the payload to the language model.     */
    int   use_web;   /* 1 => enrich with a DuckDuckGo search first.       */
} IntentResult;

/* Route a single line of user input. Caller frees with brain_free_result(). */
IntentResult brain_route(const char *input);

/* Human-readable name of an intent (for the status bar / debugging). */
const char* brain_intent_name(IntentType type);

void brain_free_result(IntentResult *res);

/* Convenience: lowercase copy of a string. Caller frees. */
char* brain_lowercase(const char *s);

#ifdef __cplusplus
}
#endif

#endif
