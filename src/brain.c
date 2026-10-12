#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "brain.h"

static const char *MATH_WORDS[] = {"calculate", "compute", "what is", "solve", "eval", "how much is"};

/* ===================================================================== */
/*  Small helpers                                                         */
/* ===================================================================== */
char* brain_lowercase(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *r = (char *)calloc(len + 1, sizeof(char));
    if (!r) return NULL;
    for (size_t i = 0; i < len; i++) r[i] = (char)tolower((unsigned char)s[i]);
    return r;
}

static int regex_match(const char *pattern, const char *text) {
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) return 0;
    int ok = (regexec(&re, text, 0, NULL, 0) == 0);
    regfree(&re);
    return ok;
}

/* ===================================================================== */
/*  Expression evaluation (shunting-yard)                                 */
/* ===================================================================== */
static double eval_expr(const char *expr, int *ok) {
    *ok = 0;
    double values[64];
    char ops[64];
    int vtop = -1, otop = -1;
    const char *p = expr;

    while (*p) {
        if (isspace((unsigned char)*p)) { p++; continue; }
        if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)p[1]))) {
            char *endp = NULL;
            double v = strtod(p, &endp);
            if (endp == p) return 0;
            if (vtop >= 63) return 0;
            values[++vtop] = v;
            p = endp;
            continue;
        }
        if (strchr("+-*/%", *p) && (otop < 63)) {
            char op = *p;
            while (otop >= 0 && vtop >= 1) {
                char top = ops[otop];
                int prec_top = (top == '+' || top == '-') ? 1 : 2;
                int prec_op  = (op == '+' || op == '-') ? 1 : 2;
                if (prec_top < prec_op) break;
                double b = values[vtop--], a = values[vtop--];
                otop--;
                double r = 0;
                switch (top) {
                    case '+': r = a + b; break;
                    case '-': r = a - b; break;
                    case '*': r = a * b; break;
                    case '/': r = (b != 0) ? a / b : 0; break;
                    case '%': r = (b != 0) ? fmod(a, b) : 0; break;
                }
                values[++vtop] = r;
            }
            ops[++otop] = op;
            p++;
            continue;
        }
        return 0; /* unsupported character */
    }
    while (otop >= 0) {
        if (vtop < 1) return 0;
        double b = values[vtop--], a = values[vtop--];
        char top = ops[otop--];
        double r = 0;
        switch (top) {
            case '+': r = a + b; break;
            case '-': r = a - b; break;
            case '*': r = a * b; break;
            case '/': r = (b != 0) ? a / b : 0; break;
            case '%': r = (b != 0) ? fmod(a, b) : 0; break;
        }
        values[++vtop] = r;
    }
    if (vtop != 0) return 0;
    *ok = 1;
    return values[0];
}

/* ===================================================================== */
/*  Intent router                                                         */
/* ===================================================================== */
static IntentResult make_result(IntentType t) {
    IntentResult r;
    memset(&r, 0, sizeof(r));
    r.type = t;
    return r;
}

static void set_text(IntentResult *r, const char *s) {
    free(r->text);
    r->text = strdup(s ? s : "");
}

static void set_payload(IntentResult *r, const char *s) {
    free(r->payload);
    r->payload = strdup(s ? s : "");
}

/* Extract the tail of the input after a leading command phrase. */
static char* extract_after(const char *input, const char *const *prefixes) {
    char *lower = brain_lowercase(input);
    if (!lower) return strdup(input);
    char *result = NULL;
    for (int i = 0; prefixes[i]; i++) {
        size_t plen = strlen(prefixes[i]);
        char *pos = strstr(lower, prefixes[i]);
        if (pos) {
            char *after = pos + plen;
            while (*after && (isspace((unsigned char)*after) || *after == ':')) after++;
            result = strdup(after);
            break;
        }
    }
    if (!result) result = strdup(input);
    free(lower);
    return result;
}

IntentResult brain_route(const char *input) {
    IntentResult r = make_result(INTENT_FALLBACK);
    if (!input || !*input) {
        set_text(&r, "I didn't catch that. Could you say it again?");
        return r;
    }

    /* 1. Unit conversion ------------------------------------------------- */
    static const char *conv_cmds[] = {"convert", NULL};
    if (regex_match("^[[:space:]]*convert[[:space:]]+.*(to|in)[[:space:]]+", input)) {
        r.type = INTENT_CONVERT;
        set_payload(&r, extract_after(input, conv_cmds));
        r.use_llm = 1;
        return r;
    }

    /* 2. Calculation (digit + operator) --------------------------------- */
    if (regex_match("[0-9]", input) && regex_match("[0-9][[:space:]]*[-+*/%][[:space:]]*[0-9]", input)) {
        r.type = INTENT_CALCULATE;
        char *expr = extract_after(input, MATH_WORDS);
        /* strip trailing punctuation and natural-language tail after the number */
        char *end = expr + strlen(expr);
        while (end > expr && !isdigit((unsigned char)end[-1])) *--end = '\0';
        int ok = 0;
        double v = eval_expr(expr, &ok);
        if (ok) {
            char buf[64];
            if (v == (long long)v) snprintf(buf, sizeof(buf), "%lld", (long long)v);
            else snprintf(buf, sizeof(buf), "%.6g", v);
            set_text(&r, buf);
        } else {
            set_text(&r, "I couldn't parse that expression.");
        }
        free(expr);
        return r;
    }

    /* 3. App control ----------------------------------------------------- */
    if (regex_match("^[[:space:]]*(help|commands|what can you do)", input)) {
        r.type = INTENT_HELP;
        set_text(&r, "Commands: help, clear, save, load, time, weather, password, roll dice, flip coin, "
                     "remind me, todo, note, pattern, ascii art, run <terminal command>, poem, story, "
                     "battle, joke, quote, fact, news, wikipedia, define, search, and anything else "
                     "goes to the LLM.");
        return r;
    }
    if (regex_match("^[[:space:]]*(clear|reset)\\b", input)) {
        r.type = INTENT_CLEAR;
        set_text(&r, "clear");
        return r;
    }
    if (regex_match("^[[:space:]]*save\\b", input)) {
        r.type = INTENT_SAVE;
        set_text(&r, "save");
        return r;
    }
    if (regex_match("^[[:space:]]*load\\b", input)) {
        r.type = INTENT_LOAD;
        set_text(&r, "load");
        return r;
    }

    /* 4. Time / weather -------------------------------------------------- */
    if (regex_match("(what time|current time|what'?s the time|the time)", input)) {
        r.type = INTENT_TIME;
        time_t t = time(NULL);
        struct tm lt;
        localtime_r(&t, &lt);
        char buf[96];
        strftime(buf, sizeof(buf), "It is %I:%M %p on %A, %d %B %Y.", &lt);
        set_text(&r, buf);
        return r;
    }
    if (regex_match("\\b(weather|forecast|temperature outside)\\b", input)) {
        r.type = INTENT_WEATHER;
        r.use_web = 1;
        r.use_llm = 1;
        set_payload(&r, input);
        return r;
    }

    /* 5. Password / dice / coin ----------------------------------------- */
    if (regex_match("\\b(password|passwd|generate a? ?password)\\b", input)) {
        r.type = INTENT_PASSWORD;
        int len = 16;
        regex_t re;
        if (regcomp(&re, "([0-9]+)", REG_EXTENDED) == 0) {
            regmatch_t m[2];
            if (regexec(&re, input, 2, m, 0) == 0) {
                int v = atoi(input + m[1].rm_so);
                if (v >= 4 && v <= 128) len = v;
            }
            regfree(&re);
        }
        static const char charset[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789!@#$%^&*()-_=+";
        char *pw = (char *)calloc((size_t)len + 1, sizeof(char));
        for (int i = 0; i < len; i++) pw[i] = charset[rand() % (int)(sizeof(charset) - 1)];
        char buf[256];
        snprintf(buf, sizeof(buf), "Generated password: %s", pw);
        set_text(&r, buf);
        free(pw);
        return r;
    }
    if (regex_match("\\b(roll|rolls|rolling)\\b.*\\b(dice|die|d[0-9]+|[0-9]+d[0-9]+)", input) ||
        regex_match("^[[:space:]]*[0-9]*d[0-9]+[[:space:]]*$", input)) {
        r.type = INTENT_ROLL;
        int count = 1, sides = 6;
        regex_t re;
        if (regcomp(&re, "([0-9]+)d([0-9]+)", REG_EXTENDED | REG_ICASE) == 0) {
            regmatch_t m[3];
            if (regexec(&re, input, 3, m, 0) == 0) {
                count = atoi(input + m[1].rm_so);
                sides = atoi(input + m[2].rm_so);
            }
            regfree(&re);
        }
        if (count < 1 || count > 100) count = 1;
        if (sides < 2 || sides > 1000) sides = 6;
        int total = 0;
        char rolls[512];
        rolls[0] = '\0';
        for (int i = 0; i < count; i++) {
            int v = (rand() % sides) + 1;
            total += v;
            char tmp[16];
            snprintf(tmp, sizeof(tmp), "%s%d", (i ? ", " : ""), v);
            strncat(rolls, tmp, sizeof(rolls) - strlen(rolls) - 1);
        }
        char buf[640];
        snprintf(buf, sizeof(buf), "Rolled %dd%d: %s (total %d)", count, sides, rolls, total);
        set_text(&r, buf);
        return r;
    }
    if (regex_match("\\b(flip|toss)\\b.*\\b(coin)\\b", input) || regex_match("^[[:space:]]*coin[[:space:]]*$", input)) {
        r.type = INTENT_FLIP;
        set_text(&r, (rand() % 2) ? "Heads!" : "Tails!");
        return r;
    }

    /* 6. Reminder / todo / note ----------------------------------------- */
    if (regex_match("^[[:space:]]*(remind me|reminder|set a reminder)", input)) {
        r.type = INTENT_REMINDER;
        set_text(&r, "Reminder noted.");
        set_payload(&r, input);
        return r;
    }
    if (regex_match("^[[:space:]]*(todo|add task|task:)", input)) {
        r.type = INTENT_TODO;
        set_text(&r, "To-do added.");
        set_payload(&r, input);
        return r;
    }
    if (regex_match("^[[:space:]]*(note|remember that|save a note)", input)) {
        r.type = INTENT_NOTE;
        set_text(&r, "Note saved.");
        set_payload(&r, input);
        return r;
    }

    /* 7. Creative / ASCII ------------------------------------------------ */
    if (regex_match("\\b(pattern|diamond|spiral|checker|waves|grid|circle|triangle|star|rainbow|gradient)\\b", input)) {
        r.type = INTENT_PATTERN;
        set_text(&r, "art");
        set_payload(&r, input);
        r.use_llm = 1; /* may be refined by the model */
        return r;
    }
    if (regex_match("\\b(ascii|ascii art|make an image|draw)\\b", input)) {
        r.type = INTENT_ASCII;
        set_text(&r, "art");
        set_payload(&r, input);
        r.use_llm = 1;
        return r;
    }
    if (regex_match("\\blyrics?\\b", input)) {
        r.type = INTENT_LYRICS;
        set_payload(&r, input);
        r.use_llm = 1;
        return r;
    }

    /* 8. Web-backed lookups -------------------------------------------- */
    if (regex_match("\\b(news|headlines|latest on|what'?s happening)\\b", input)) {
        r.type = INTENT_NEWS;
        set_payload(&r, input);
        r.use_web = 1;
        r.use_llm = 1;
        r.text = strdup("");
        return r;
    }
    if (regex_match("\\b(wikipedia|who is|who was)\\b", input)) {
        r.type = INTENT_WIKIPEDIA;
        set_payload(&r, input);
        r.use_web = 1;
        r.use_llm = 1;
        return r;
    }
    if (regex_match("\\b(search for|look up|google|find information on|search)\\b", input)) {
        r.type = INTENT_WEB_SEARCH;
        char *q = extract_after(input, (const char *[]){"search for", "look up", "google", "search", "find information on", NULL});
        set_payload(&r, q);
        free(q);
        r.use_web = 1;
        r.use_llm = 1;
        return r;
    }

    /* 9. Terminal command ("run ..." or "$ ...") ------------------------ */
    if (regex_match("^[[:space:]]*(run[[:space:]]+|[$!][[:space:]]*)", input)) {
        const char *p = input;
        while (*p && isspace((unsigned char)*p)) p++;
        if (strncmp(p, "run", 3) == 0 && isspace((unsigned char)p[3])) {
            p += 3;                       /* skip the word "run"            */
        } else {
            p++;                          /* skip the leading $ or !        */
        }
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p) {
            r.type = INTENT_TERMINAL;
            set_payload(&r, p);
            return r;
        }
    }

    /* 10. Fallback -> LLM ----------------------------------------------- */
    r.type = INTENT_FALLBACK;
    set_payload(&r, input);
    r.use_llm = 1;
    return r;
}

const char* brain_intent_name(IntentType type) {
    switch (type) {
        case INTENT_CONVERT:      return "convert";
        case INTENT_CALCULATE:    return "calculate";
        case INTENT_GREETING:     return "greeting";
        case INTENT_FAREWELL:     return "farewell";
        case INTENT_NAME_ASK:     return "name_ask";
        case INTENT_NAME_GIVE:    return "name_give";
        case INTENT_HOW_ARE_YOU:  return "how_are_you";
        case INTENT_HELP:         return "help";
        case INTENT_CLEAR:        return "clear";
        case INTENT_SAVE:         return "save";
        case INTENT_LOAD:         return "load";
        case INTENT_TIME:         return "time";
        case INTENT_WEATHER:      return "weather";
        case INTENT_PASSWORD:     return "password";
        case INTENT_ROLL:         return "roll";
        case INTENT_FLIP:         return "flip";
        case INTENT_REMINDER:     return "reminder";
        case INTENT_TODO:         return "todo";
        case INTENT_NOTE:         return "note";
        case INTENT_PATTERN:      return "pattern";
        case INTENT_ASCII:        return "ascii";
        case INTENT_LYRICS:       return "lyrics";
        case INTENT_POEM:         return "poem";
        case INTENT_STORY:        return "story";
        case INTENT_BATTLE:       return "battle";
        case INTENT_JOKE:         return "joke";
        case INTENT_QUOTE:        return "quote";
        case INTENT_FACT:         return "fact";
        case INTENT_NEWS:         return "news";
        case INTENT_WIKIPEDIA:    return "wikipedia";
        case INTENT_SEARCH:       return "search";
        case INTENT_DEFINE:       return "define";
        case INTENT_WEB_SEARCH:   return "web_search";
        case INTENT_TERMINAL:     return "terminal";
        case INTENT_FALLBACK:     return "llm";
        default:                  return "none";
    }
}

void brain_free_result(IntentResult *res) {
    if (!res) return;
    free(res->text);
    free(res->payload);
    res->text = NULL;
    res->payload = NULL;
}
