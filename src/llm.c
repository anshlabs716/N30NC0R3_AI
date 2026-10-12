#include "llm.h"

static struct llama_model   *g_model  = NULL;
static struct llama_context *g_ctx    = NULL;
static const struct llama_vocab *g_vocab = NULL;
static int g_loaded = 0;

#define SYSTEM_PROMPT "You are NEON, a helpful AI assistant."
#define MAX_HISTORY_MSGS 20

typedef struct {
    char role[16];
    char *text;
} ChatMsg;

static ChatMsg g_history[MAX_HISTORY_MSGS];
static int g_history_count = 0;

int llm_is_loaded(void) {
    return g_loaded;
}

int llm_init(void) {
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
    if (!model_path) {
        return 0;
    }

    llama_backend_init();

    struct llama_model_params mparams = llama_model_default_params();
    mparams.n_gpu_layers = 0;
    g_model = llama_model_load_from_file(model_path, mparams);
    if (!g_model) {
        llama_backend_free();
        return 0;
    }

    struct llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = 2048;
    long procs = sysconf(_SC_NPROCESSORS_ONLN);
    int n_threads = (int)(procs / 2);
    if (n_threads < 1) n_threads = 1;
    cparams.n_threads = n_threads;
    cparams.n_threads_batch = n_threads;
    cparams.n_batch = 512;

    g_ctx = llama_init_from_model(g_model, cparams);
    if (!g_ctx) {
        llama_model_free(g_model);
        g_model = NULL;
        llama_backend_free();
        return 0;
    }

    g_vocab = llama_model_get_vocab(g_model);
    g_loaded = 1;
    return 1;
}

void llm_cleanup(void) {
    if (g_ctx) {
        llama_free(g_ctx);
        g_ctx = NULL;
    }
    if (g_model) {
        llama_model_free(g_model);
        g_model = NULL;
    }
    g_vocab = NULL;
    for (int i = 0; i < g_history_count; i++) {
        free(g_history[i].text);
        g_history[i].text = NULL;
    }
    g_history_count = 0;
    if (g_loaded) {
        llama_backend_free();
    }
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
    llama_sampler_chain_add(smpl, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(smpl, llama_sampler_init_top_p(0.9f, 1));
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.7f));
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

char *llm_generate(const char *prompt, llm_stream_callback cb, void *data) {
    if (!g_loaded || !g_ctx || !prompt) {
        return NULL;
    }

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

    int n_decode = 0;
    int n_max = n_ctx - n_prompt;
    if (n_max > 1024) n_max = 1024;

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
        n_decode++;
    }

    llama_sampler_free(smpl);
    free(tokens);

    /* record in sliding-window history */
    history_add("user", prompt);
    history_add("assistant", out);

    if (out_len == 0) {
        free(out);
        return strdup("");
    }
    return out;
}

char *llm_generate_sync(const char *prompt) {
    if (!g_loaded) {
        return strdup("The language model is not loaded. Place a GGUF model in the models/ directory.");
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
