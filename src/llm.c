#include "llm.h"

#include <curl/curl.h>
#include "cJSON.h"

#define SYSTEM_PROMPT "You are neoncore, a friendly and helpful AI assistant. " \
                      "Answer clearly and stay on topic. Keep replies short " \
                      "(usually one to three sentences) and never repeat yourself."
#define MAX_HISTORY_MSGS 20

/* Ollama endpoint and model used when no local GGUF is present. */
#define OLLAMA_HOST  "http://127.0.0.1:11434"
#define OLLAMA_MODEL "qwen3:1.7b"

/* Free, keyless cloud backend (OpenAI-compatible). */
#define CLOUD_ENDPOINT "https://text.pollinations.ai/openai"
#define CLOUD_MODEL    "openai-fast"

typedef enum {
    BACKEND_NONE = 0,
    BACKEND_LLAMA,   /* direct in-process inference through llama.cpp */
    BACKEND_OLLAMA,  /* HTTP inference through a local Ollama server      */
    BACKEND_CLOUD    /* cloud inference (keyless or with an API key)      */
} LlmBackend;

static struct llama_model   *g_model  = NULL;
static struct llama_context *g_ctx    = NULL;
static const struct llama_vocab *g_vocab = NULL;
static LlmBackend g_backend = BACKEND_NONE;
static int g_loaded = 0;
static int g_curl_ready = 0;

/* Cloud configuration (set from Settings before llm_init). */
static int  g_cloud_enabled = 1;
static char g_cloud_endpoint[512] = CLOUD_ENDPOINT;
static char g_cloud_model[128]    = CLOUD_MODEL;
static char g_cloud_key[512]      = "";
static char g_cloud_label[32]     = "pollinations";

void llm_set_cloud_enabled(int enabled) {
    g_cloud_enabled = enabled ? 1 : 0;
}

void llm_set_cloud_config(const char *endpoint, const char *model,
                          const char *api_key, const char *label) {
    snprintf(g_cloud_endpoint, sizeof(g_cloud_endpoint), "%s",
             (endpoint && endpoint[0]) ? endpoint : CLOUD_ENDPOINT);
    snprintf(g_cloud_model, sizeof(g_cloud_model), "%s",
             (model && model[0]) ? model : CLOUD_MODEL);
    snprintf(g_cloud_key, sizeof(g_cloud_key), "%s", api_key ? api_key : "");
    snprintf(g_cloud_label, sizeof(g_cloud_label), "%s",
             (label && label[0]) ? label : "cloud");
}

typedef struct {
    char role[16];
    char *text;
} ChatMsg;

static ChatMsg g_history[MAX_HISTORY_MSGS];
static int g_history_count = 0;

static void history_add(const char *role, const char *text);

int llm_is_loaded(void) {
    return g_loaded;
}

const char *llm_backend_name(void) {
    static char cloud_name[64];
    switch (g_backend) {
        case BACKEND_LLAMA:  return "llama.cpp";
        case BACKEND_OLLAMA: return "ollama";
        case BACKEND_CLOUD:
            snprintf(cloud_name, sizeof(cloud_name), "cloud (%s)", g_cloud_label);
            return cloud_name;
        default:             return "none";
    }
}

/* --------------------------------------------------------------------- */
/*  Ollama HTTP backend                                                  */
/* --------------------------------------------------------------------- */

struct ollama_buf {
    char  *data;
    size_t size;
};

static size_t ollama_buf_write(char *ptr, size_t size, size_t nmemb, void *ud) {
    struct ollama_buf *b = (struct ollama_buf *)ud;
    size_t total = size * nmemb;
    char *grown = (char *)realloc(b->data, b->size + total + 1);
    if (!grown) return 0;
    b->data = grown;
    memcpy(b->data + b->size, ptr, total);
    b->size += total;
    b->data[b->size] = '\0';
    return total;
}

/* Ping the local Ollama server and look for a loaded qwen3 model. */
static int ollama_detect(void) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    struct ollama_buf buf = {0};
    buf.data = (char *)calloc(1, 1);

    curl_easy_setopt(curl, CURLOPT_URL, OLLAMA_HOST "/api/tags");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ollama_buf_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 6L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode rc = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);

    int ok = (rc == CURLE_OK && code == 200);
    /* The server is only useful if the model has actually been pulled. */
    if (ok && buf.data && !strstr(buf.data, "qwen3")) ok = 0;
    free(buf.data);
    return ok;
}

/* Streaming NDJSON parser state for /api/chat. */
typedef struct {
    char  *line;
    size_t len, cap;
    llm_stream_callback cb;
    void  *cbdata;
    char  *out;
    size_t outlen, outcap;
} OllamaStream;

static void ollama_append_out(OllamaStream *st, const char *s) {
    size_t n = strlen(s);
    if (st->outlen + n + 1 > st->outcap) {
        size_t nc = st->outcap ? st->outcap * 2 : 4096;
        while (nc < st->outlen + n + 1) nc *= 2;
        char *grown = (char *)realloc(st->out, nc);
        if (!grown) return;
        st->out = grown;
        st->outcap = nc;
    }
    memcpy(st->out + st->outlen, s, n);
    st->outlen += n;
    st->out[st->outlen] = '\0';
}

/* Handle one complete JSON line from the stream. */
static void ollama_handle_line(OllamaStream *st) {
    if (st->len == 0) return;
    st->line[st->len] = '\0';
    st->len = 0;

    cJSON *obj = cJSON_Parse(st->line);
    if (!obj) return;

    cJSON *err = cJSON_GetObjectItemCaseSensitive(obj, "error");
    if (cJSON_IsString(err) && err->valuestring) {
        char msg[2048];
        snprintf(msg, sizeof(msg), "\n[ollama] %s\n", err->valuestring);
        ollama_append_out(st, msg);
        if (st->cb) st->cb(msg, st->cbdata);
    } else {
        cJSON *message = cJSON_GetObjectItemCaseSensitive(obj, "message");
        cJSON *content = message ? cJSON_GetObjectItemCaseSensitive(message, "content") : NULL;
        if (cJSON_IsString(content) && content->valuestring && content->valuestring[0]) {
            ollama_append_out(st, content->valuestring);
            if (st->cb) st->cb(content->valuestring, st->cbdata);
        }
    }
    cJSON_Delete(obj);
}

static size_t ollama_stream_cb(char *ptr, size_t size, size_t nmemb, void *ud) {
    OllamaStream *st = (OllamaStream *)ud;
    size_t total = size * nmemb;
    for (size_t i = 0; i < total; i++) {
        char ch = ptr[i];
        if (ch == '\n') { ollama_handle_line(st); continue; }
        if (ch == '\r') continue;
        if (st->len + 2 > st->cap) {
            size_t nc = st->cap ? st->cap * 2 : 1024;
            char *grown = (char *)realloc(st->line, nc);
            if (!grown) return 0;
            st->line = grown;
            st->cap = nc;
        }
        st->line[st->len++] = ch;
    }
    return total;
}

static char *ollama_build_body(const char *user_input) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    cJSON_AddStringToObject(root, "model", OLLAMA_MODEL);

    cJSON *messages = cJSON_AddArrayToObject(root, "messages");
    cJSON *sys = cJSON_CreateObject();
    cJSON_AddStringToObject(sys, "role", "system");
    cJSON_AddStringToObject(sys, "content", SYSTEM_PROMPT);
    cJSON_AddItemToArray(messages, sys);

    for (int i = 0; i < g_history_count; i++) {
        cJSON *m = cJSON_CreateObject();
        cJSON_AddStringToObject(m, "role", g_history[i].role);
        cJSON_AddStringToObject(m, "content", g_history[i].text ? g_history[i].text : "");
        cJSON_AddItemToArray(messages, m);
    }

    cJSON *um = cJSON_CreateObject();
    cJSON_AddStringToObject(um, "role", "user");
    cJSON_AddStringToObject(um, "content", user_input ? user_input : "");
    cJSON_AddItemToArray(messages, um);

    cJSON *options = cJSON_AddObjectToObject(root, "options");
    if (options) {
        cJSON_AddNumberToObject(options, "temperature", 0.6);
        cJSON_AddNumberToObject(options, "top_p", 0.95);
        cJSON_AddNumberToObject(options, "top_k", 20);
        cJSON_AddNumberToObject(options, "repeat_penalty", 1.15);
        cJSON_AddNumberToObject(options, "repeat_last_n", 64);
        cJSON_AddNumberToObject(options, "num_predict", 512);
    }

    cJSON_AddBoolToObject(root, "stream", 1);
    /* Qwen3 supports thinking; turn it off so replies are short and direct. */
    cJSON_AddBoolToObject(root, "think", 0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return body;
}

static char *ollama_generate(const char *prompt, llm_stream_callback cb, void *data) {
    char *body = ollama_build_body(prompt);
    if (!body) return NULL;

    CURL *curl = curl_easy_init();
    if (!curl) {
        free(body);
        return strdup("");
    }

    OllamaStream st;
    memset(&st, 0, sizeof(st));
    st.cb = cb;
    st.cbdata = data;

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, OLLAMA_HOST "/api/chat");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ollama_stream_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &st);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode rc = curl_easy_perform(curl);
    ollama_handle_line(&st); /* flush a trailing line without newline */

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(body);
    free(st.line);

    if (rc != CURLE_OK && (!st.out || !st.out[0])) {
        const char *msg = "I couldn't reach the local Ollama server. "
                          "Use the \"Get LLM\" button to install it and pull the model.";
        if (cb) cb(msg, data);
        free(st.out);
        return strdup(msg);
    }

    history_add("user", prompt);
    history_add("assistant", st.out ? st.out : "");

    if (!st.out || !st.out[0]) {
        free(st.out);
        return strdup("");
    }
    return st.out;
}

/* --------------------------------------------------------------------- */
/*  Cloud HTTP backend (OpenAI-compatible, keyless by default)           */
/* --------------------------------------------------------------------- */

/* Check that the cloud backend looks usable. The free keyless endpoint is
 * probed for real; keyed/custom endpoints are assumed reachable and any
 * problem (bad key, offline) is reported when the request is made. */
static int cloud_detect(void) {
    int is_keyless_default =
        (strcmp(g_cloud_endpoint, CLOUD_ENDPOINT) == 0) && (g_cloud_key[0] == '\0');
    if (!is_keyless_default) return g_cloud_endpoint[0] ? 1 : 0;

    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    struct ollama_buf buf = {0};
    buf.data = (char *)calloc(1, 1);

    curl_easy_setopt(curl, CURLOPT_URL, "https://text.pollinations.ai/models");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ollama_buf_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 4L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "neoncore/3.0");

    CURLcode rc = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);
    free(buf.data);
    return (rc == CURLE_OK && code == 200);
}

typedef struct {
    char  *line;
    size_t len, cap;
    llm_stream_callback cb;
    void  *cbdata;
    char  *out;
    size_t outlen, outcap;
    int    denied;   /* set when the server returns a rate-limit / auth error */
} CloudStream;

static void cloud_append_out(CloudStream *st, const char *s) {
    size_t n = strlen(s);
    if (st->outlen + n + 1 > st->outcap) {
        size_t nc = st->outcap ? st->outcap * 2 : 4096;
        while (nc < st->outlen + n + 1) nc *= 2;
        char *grown = (char *)realloc(st->out, nc);
        if (!grown) return;
        st->out = grown;
        st->outcap = nc;
    }
    memcpy(st->out + st->outlen, s, n);
    st->outlen += n;
    st->out[st->outlen] = '\0';
}

/* Parse one Server-Sent-Events line, i.e. `data: {json}`. */
static void cloud_handle_line(CloudStream *st) {
    if (st->len == 0) return;
    st->line[st->len] = '\0';
    st->len = 0;

    const char *payload = st->line;
    if (strncmp(payload, "data:", 5) != 0) return;  /* keep-alive / comment */
    payload += 5;
    while (*payload == ' ') payload++;
    if (strncmp(payload, "[DONE]", 6) == 0) return;

    cJSON *obj = cJSON_Parse(payload);
    if (!obj) return;

    cJSON *err = cJSON_GetObjectItemCaseSensitive(obj, "error");
    if (err && !cJSON_IsNull(err)) {
        const char *etext = cJSON_IsString(err) ? err->valuestring : NULL;
        if (!etext && cJSON_IsObject(err)) {
            cJSON *m = cJSON_GetObjectItemCaseSensitive(err, "message");
            if (cJSON_IsString(m)) etext = m->valuestring;
        }
        char msg[2048];
        snprintf(msg, sizeof(msg), "\n[cloud] %s\n", etext ? etext : "request failed");
        cloud_append_out(st, msg);
        if (st->cb) st->cb(msg, st->cbdata);
        st->denied = 1;
        cJSON_Delete(obj);
        return;
    }

    cJSON *choices = cJSON_GetObjectItemCaseSensitive(obj, "choices");
    cJSON *choice0 = cJSON_IsArray(choices) ? cJSON_GetArrayItem(choices, 0) : NULL;
    cJSON *delta   = choice0 ? cJSON_GetObjectItemCaseSensitive(choice0, "delta") : NULL;
    cJSON *content = delta ? cJSON_GetObjectItemCaseSensitive(delta, "content") : NULL;
    if (cJSON_IsString(content) && content->valuestring && content->valuestring[0]) {
        cloud_append_out(st, content->valuestring);
        if (st->cb) st->cb(content->valuestring, st->cbdata);
    }
    cJSON_Delete(obj);
}

static size_t cloud_stream_cb(char *ptr, size_t size, size_t nmemb, void *ud) {
    CloudStream *st = (CloudStream *)ud;
    size_t total = size * nmemb;
    for (size_t i = 0; i < total; i++) {
        char ch = ptr[i];
        if (ch == '\n') { cloud_handle_line(st); continue; }
        if (ch == '\r') continue;
        if (st->len + 2 > st->cap) {
            size_t nc = st->cap ? st->cap * 2 : 1024;
            char *grown = (char *)realloc(st->line, nc);
            if (!grown) return 0;
            st->line = grown;
            st->cap = nc;
        }
        st->line[st->len++] = ch;
    }
    return total;
}

static char *cloud_build_body(const char *user_input) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    cJSON_AddStringToObject(root, "model", g_cloud_model);

    cJSON *messages = cJSON_AddArrayToObject(root, "messages");
    cJSON *sys = cJSON_CreateObject();
    cJSON_AddStringToObject(sys, "role", "system");
    cJSON_AddStringToObject(sys, "content", SYSTEM_PROMPT);
    cJSON_AddItemToArray(messages, sys);

    for (int i = 0; i < g_history_count; i++) {
        cJSON *m = cJSON_CreateObject();
        cJSON_AddStringToObject(m, "role", g_history[i].role);
        cJSON_AddStringToObject(m, "content", g_history[i].text ? g_history[i].text : "");
        cJSON_AddItemToArray(messages, m);
    }

    cJSON *um = cJSON_CreateObject();
    cJSON_AddStringToObject(um, "role", "user");
    cJSON_AddStringToObject(um, "content", user_input ? user_input : "");
    cJSON_AddItemToArray(messages, um);

    cJSON_AddBoolToObject(root, "stream", 1);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return body;
}

static char *cloud_generate(const char *prompt, llm_stream_callback cb, void *data) {
    char *body = cloud_build_body(prompt);
    if (!body) return NULL;

    CURL *curl = curl_easy_init();
    if (!curl) { free(body); return strdup(""); }

    CloudStream st;
    memset(&st, 0, sizeof(st));
    st.cb = cb;
    st.cbdata = data;

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    char auth[600];
    if (g_cloud_key[0]) {
        snprintf(auth, sizeof(auth), "Authorization: Bearer %s", g_cloud_key);
        headers = curl_slist_append(headers, auth);
    }

    curl_easy_setopt(curl, CURLOPT_URL, g_cloud_endpoint);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cloud_stream_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &st);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "neoncore/3.0");

    CURLcode rc = curl_easy_perform(curl);
    cloud_handle_line(&st);   /* flush a trailing line without newline */

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(body);
    free(st.line);

    if (rc != CURLE_OK && (!st.out || !st.out[0])) {
        const char *msg = "I couldn't reach the cloud AI. Check your internet "
                          "connection, or turn off cloud AI in Settings to use a "
                          "local model.";
        if (cb) cb(msg, data);
        free(st.out);
        return strdup(msg);
    }

    history_add("user", prompt);
    history_add("assistant", st.out ? st.out : "");

    if (!st.out || !st.out[0]) {
        free(st.out);
        if (st.denied) {
            const char *m = "The cloud AI said no to that request (busy or bad key). "
                            "Wait a moment, or add/check your key in Settings.";
            if (cb) cb(m, data);
            return strdup(m);
        }
        return strdup("");
    }
    return st.out;
}

/* --------------------------------------------------------------------- */
/*  llama.cpp in-process backend                                         */
/* --------------------------------------------------------------------- */

int llm_init(void) {
    if (!g_curl_ready) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        g_curl_ready = 1;
    }

    /* Prefer the cloud backend when it is enabled and looks reachable. */
    if (g_cloud_enabled && cloud_detect()) {
        g_backend = BACKEND_CLOUD;
        g_loaded = 1;
        return 1;
    }

    const char *model_paths[] = {
        "./models/qwen3-1.7b-q4_k_m.gguf",
        "./models/Qwen3-1.7B-Q4_K_M.gguf",
        "./models/Qwen3-1.7B-Q4_K_M-00001-of-00001.gguf",
        "./models/model.gguf",
        NULL
    };

    const char *model_path = NULL;
    for (int i = 0; model_paths[i]; i++) {
        FILE *f = fopen(model_paths[i], "r");
        if (f) {
            model_path = model_paths[i];
            fclose(f);
            break;
        }
    }

    if (model_path) {
        llama_backend_init();

        struct llama_model_params mparams = llama_model_default_params();
        mparams.n_gpu_layers = 0;
        g_model = llama_model_load_from_file(model_path, mparams);

        if (g_model) {
            struct llama_context_params cparams = llama_context_default_params();
            cparams.n_ctx = 2048;
            long procs = sysconf(_SC_NPROCESSORS_ONLN);
            int n_threads = (int)(procs / 2);
            if (n_threads < 1) n_threads = 1;
            cparams.n_threads = n_threads;
            cparams.n_threads_batch = n_threads;
            cparams.n_batch = 512;

            g_ctx = llama_init_from_model(g_model, cparams);
            if (g_ctx) {
                g_vocab = llama_model_get_vocab(g_model);
                g_backend = BACKEND_LLAMA;
                g_loaded = 1;
                return 1;
            }
            llama_model_free(g_model);
            g_model = NULL;
        }
        llama_backend_free();
    }

    /* No usable GGUF: fall back to a local Ollama server if one is running. */
    if (ollama_detect()) {
        g_backend = BACKEND_OLLAMA;
        g_loaded = 1;
        return 1;
    }

    return 0;
}

void llm_cleanup(void) {
    if (g_backend == BACKEND_LLAMA) {
        if (g_ctx) llama_free(g_ctx);
        if (g_model) llama_model_free(g_model);
        llama_backend_free();
    }
    g_ctx = NULL;
    g_model = NULL;
    g_vocab = NULL;

    for (int i = 0; i < g_history_count; i++) {
        free(g_history[i].text);
        g_history[i].text = NULL;
    }
    g_history_count = 0;

    g_backend = BACKEND_NONE;
    g_loaded = 0;
}

static void history_add(const char *role, const char *text) {
    if (g_history_count >= MAX_HISTORY_MSGS) {
        free(g_history[0].text);
        memmove(&g_history[0], &g_history[1], sizeof(ChatMsg) * (MAX_HISTORY_MSGS - 1));
        g_history_count = MAX_HISTORY_MSGS - 1;
    }
    snprintf(g_history[g_history_count].role, sizeof(g_history[0].role), "%s", role);
    g_history[g_history_count].text = strdup(text ? text : "");
    g_history_count++;
}

static struct llama_sampler *make_sampler(void) {
    struct llama_sampler *smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    int32_t n_vocab = g_vocab ? llama_vocab_n_tokens(g_vocab) : 0;
    /* Repetition penalty is the important one: it stops the model looping. */
    llama_sampler_chain_add(smpl, llama_sampler_init_penalties(n_vocab, 64, 1.15f, 0.0f, 0.0f));
    llama_sampler_chain_add(smpl, llama_sampler_init_top_k(20));
    llama_sampler_chain_add(smpl, llama_sampler_init_top_p(0.95f, 1));
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.6f));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    return smpl;
}

static char *build_prompt(const char *user_input) {
    /* Build a simple ChatML-style prompt with a sliding window of history. */
    size_t cap = 8192;
    char *buf = calloc(cap, sizeof(char));
    if (!buf) return NULL;
    buf[0] = '\0';

    strncat(buf, "<|im_start|>system\n" SYSTEM_PROMPT "<|im_end|>\n", cap - strlen(buf) - 1);

    for (int i = 0; i < g_history_count; i++) {
        char line[2400];
        if (strcmp(g_history[i].role, "user") == 0) {
            snprintf(line, sizeof(line), "<|im_start|>user\n%s<|im_end|>\n", g_history[i].text);
        } else {
            snprintf(line, sizeof(line), "<|im_start|>assistant\n%s<|im_end|>\n", g_history[i].text);
        }
        if (strlen(buf) + strlen(line) + 64 >= cap) break;
        strcat(buf, line);
    }

    char line[2400];
    snprintf(line, sizeof(line), "<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n",
             user_input ? user_input : "");
    if (strlen(buf) + strlen(line) + 16 >= cap) {
        /* trim oldest to fit */
        buf[0] = '\0';
        strncat(buf, "<|im_start|>system\n" SYSTEM_PROMPT "<|im_end|>\n", cap - 1);
    }
    strncat(buf, line, cap - strlen(buf) - 1);
    return buf;
}

static char *llama_generate(const char *prompt, llm_stream_callback cb, void *data) {
    char *full = build_prompt(prompt);
    if (!full) return NULL;

    int n_prompt = -llama_tokenize(g_vocab, full, (int32_t)strlen(full), NULL, 0, true, true);
    if (n_prompt <= 0) {
        free(full);
        return strdup("");
    }
    llama_token *tokens = calloc((size_t)n_prompt, sizeof(llama_token));
    if (!tokens) {
        free(full);
        return strdup("");
    }
    if (llama_tokenize(g_vocab, full, (int32_t)strlen(full), tokens, n_prompt, true, true) < 0) {
        free(tokens);
        free(full);
        return strdup("");
    }
    free(full);

    llama_memory_clear(llama_get_memory(g_ctx), true);

    struct llama_sampler *smpl = make_sampler();

    /* cap prompt to available context, keeping the most recent tokens */
    int n_ctx = (int)llama_n_ctx(g_ctx);
    if (n_prompt > n_ctx - 64) {
        int drop = n_prompt - (n_ctx - 64);
        memmove(tokens, tokens + drop, sizeof(llama_token) * (size_t)(n_prompt - drop));
        n_prompt -= drop;
    }

    struct llama_batch batch = llama_batch_get_one(tokens, n_prompt);

    size_t out_cap = 16384;
    char *out = calloc(out_cap, sizeof(char));
    size_t out_len = 0;

    int n_max = n_ctx - n_prompt;
    if (n_max > 512) n_max = 512;   /* keep answers short and stop rambling */

    for (int i = 0; i < n_max; i++) {
        if (llama_decode(g_ctx, batch) != 0) break;

        llama_token id = llama_sampler_sample(smpl, g_ctx, -1);
        if (llama_vocab_is_eog(g_vocab, id)) break;

        char piece[256];
        int np = llama_token_to_piece(g_vocab, id, piece, sizeof(piece), 0, true);
        if (np < 0) break;
        piece[np] = '\0';

        if (out_len + (size_t)np + 1 < out_cap) {
            memcpy(out + out_len, piece, (size_t)np);
            out_len += (size_t)np;
            out[out_len] = '\0';
        } else {
            break;
        }

        if (cb) cb(piece, data);

        batch = llama_batch_get_one(&id, 1);
    }

    llama_sampler_free(smpl);
    free(tokens);

    history_add("user", prompt);
    history_add("assistant", out);

    if (out_len == 0) {
        free(out);
        return strdup("");
    }
    return out;
}

char *llm_generate(const char *prompt, llm_stream_callback cb, void *data) {
    if (!g_loaded || !prompt) return NULL;

    if (g_backend == BACKEND_OLLAMA) {
        return ollama_generate(prompt, cb, data);
    }
    if (g_backend == BACKEND_CLOUD) {
        return cloud_generate(prompt, cb, data);
    }
    if (g_backend == BACKEND_LLAMA && g_ctx) {
        return llama_generate(prompt, cb, data);
    }
    return NULL;
}

char *llm_generate_sync(const char *prompt) {
    if (!g_loaded) {
        return strdup("The language model is not loaded. Use the \"Get LLM\" button to set it up.");
    }
    char *r = llm_generate(prompt, NULL, NULL);
    if (r) return r;
    return strdup("");
}

void llm_reset_history(void) {
    for (int i = 0; i < g_history_count; i++) {
        free(g_history[i].text);
        g_history[i].text = NULL;
    }
    g_history_count = 0;
}
