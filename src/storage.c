#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "storage.h"

#include <time.h>
#include <ctype.h>

void storage_dir(char *out, size_t n) {
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(out, n, "%s/.neon_core_ai", home);
}

static void ensure_dir(void) {
    char dir[512];
    storage_dir(dir, sizeof(dir));
    mkdir(dir, 0700);
}

static void now_string(char *out, size_t n) {
    time_t t = time(NULL);
    struct tm lt;
    localtime_r(&t, &lt);
    strftime(out, n, "%Y-%m-%d %H:%M:%S", &lt);
}

static char* slugify(const char *s) {
    char *out = (char *)calloc(strlen(s ? s : "chat") + 1, sizeof(char));
    if (!out) return strdup("chat");
    size_t j = 0;
    for (size_t i = 0; s && s[i]; i++) {
        if (isalnum((unsigned char)s[i])) out[j++] = s[i];
        else if (s[i] == ' ') out[j++] = ' ';
    }
    while (j > 0 && out[j - 1] == ' ') out[j - 1] = '\0';
    /* keep spaces replaced with underscores */
    for (size_t i = 0; out[i]; i++) if (out[i] == ' ') out[i] = '_';
    if (!out[0]) { free(out); return strdup("chat"); }
    return out;
}

/* --------------------------------------------------------------------- */
/*  Chat construction                                                     */
/* --------------------------------------------------------------------- */
static Chat* chat_new(const char *id, const char *name) {
    Chat *c = (Chat *)calloc(1, sizeof(Chat));
    if (!c) return NULL;
    c->id = strdup(id);
    c->name = strdup(name && *name ? name : "New Chat");
    char t[64];
    now_string(t, sizeof(t));
    c->time = strdup(t);
    c->msg_cap = 16;
    c->messages = (Message *)calloc((size_t)c->msg_cap, sizeof(Message));
    c->msg_count = 0;
    return c;
}

Chat* storage_create_chat(History *h, const char *name) {
    if (!h) return NULL;
    if (h->chat_count >= h->chat_cap) {
        int nc = h->chat_cap ? h->chat_cap * 2 : 8;
        Chat *grown = (Chat *)realloc(h->chats, (size_t)nc * sizeof(Chat));
        if (!grown) return NULL;
        h->chats = grown;
        h->chat_cap = nc;
    }
    char id[32];
    snprintf(id, sizeof(id), "%d", h->next_id++);
    Chat *c = chat_new(id, name);
    if (!c) return NULL;
    h->chats[h->chat_count++] = *c;
    free(c); /* struct copied into the array; strings are now owned by h */
    return &h->chats[h->chat_count - 1];
}

static void free_chat_contents(Chat *c) {
    free(c->id);
    free(c->name);
    free(c->time);
    for (int i = 0; i < c->msg_count; i++) {
        free(c->messages[i].role);
        free(c->messages[i].text);
        free(c->messages[i].time);
    }
    free(c->messages);
    memset(c, 0, sizeof(*c));
}

int storage_delete_chat(History *h, const char *id) {
    if (!h || !id) return 0;
    for (int i = 0; i < h->chat_count; i++) {
        if (strcmp(h->chats[i].id, id) == 0) {
            free_chat_contents(&h->chats[i]);
            memmove(&h->chats[i], &h->chats[i + 1],
                    (size_t)(h->chat_count - i - 1) * sizeof(Chat));
            h->chat_count--;
            return 1;
        }
    }
    return 0;
}

int storage_rename_chat(History *h, const char *id, const char *name) {
    if (!h || !id || !name) return 0;
    for (int i = 0; i < h->chat_count; i++) {
        if (strcmp(h->chats[i].id, id) == 0) {
            free(h->chats[i].name);
            h->chats[i].name = strdup(name);
            return 1;
        }
    }
    return 0;
}

void storage_clear_all(History *h) {
    if (!h) return;
    for (int i = 0; i < h->chat_count; i++) free_chat_contents(&h->chats[i]);
    h->chat_count = 0;
    h->next_id = 1;
}

int storage_add_message(Chat *c, const char *role, const char *text) {
    if (!c || !role || !text) return 0;
    if (c->msg_count >= c->msg_cap) {
        int nc = c->msg_cap ? c->msg_cap * 2 : 16;
        if (nc > STORAGE_MAX_MESSAGES) nc = STORAGE_MAX_MESSAGES;
        if (c->msg_count >= nc) return 0;
        Message *grown = (Message *)realloc(c->messages, (size_t)nc * sizeof(Message));
        if (!grown) return 0;
        c->messages = grown;
        c->msg_cap = nc;
    }
    Message *m = &c->messages[c->msg_count++];
    m->role = strdup(role);
    m->text = strdup(text);
    char t[64];
    now_string(t, sizeof(t));
    m->time = strdup(t);
    return 1;
}

/* --------------------------------------------------------------------- */
/*  Persistence                                                           */
/* --------------------------------------------------------------------- */
History* storage_load(void) {
    ensure_dir();
    History *h = (History *)calloc(1, sizeof(History));
    if (!h) return NULL;
    h->next_id = 1;

    char dir[512], path[600];
    storage_dir(dir, sizeof(dir));
    snprintf(path, sizeof(path), "%s/history.json", dir);

    FILE *f = fopen(path, "r");
    if (!f) return h;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || size > 32 * 1024 * 1024) { fclose(f); return h; }
    char *buf = (char *)calloc((size_t)size + 1, sizeof(char));
    if (!buf) { fclose(f); return h; }
    size_t rd = fread(buf, 1, (size_t)size, f);
    buf[rd] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return h;

    cJSON *next_id = cJSON_GetObjectItem(root, "next_id");
    if (next_id && cJSON_IsNumber(next_id)) h->next_id = next_id->valueint;

    cJSON *chats = cJSON_GetObjectItem(root, "chats");
    cJSON *messages = cJSON_GetObjectItem(root, "messages");
    if (chats && cJSON_IsObject(chats)) {
        cJSON *chat;
        cJSON_ArrayForEach(chat, chats) {
            if (!chat->string) continue;
            Chat *c = storage_create_chat(h, chat->string);
            if (!c) continue;
            free(c->name);
            cJSON *nm = cJSON_GetObjectItem(chat, "name");
            c->name = strdup(nm && cJSON_IsString(nm) ? nm->valuestring : chat->string);
            cJSON *tm = cJSON_GetObjectItem(chat, "time");
            if (tm && cJSON_IsString(tm)) { free(c->time); c->time = strdup(tm->valuestring); }

            if (messages && cJSON_IsObject(messages)) {
                cJSON *arr = cJSON_GetObjectItem(messages, chat->string);
                if (arr && cJSON_IsArray(arr)) {
                    cJSON *m;
                    cJSON_ArrayForEach(m, arr) {
                        cJSON *role = cJSON_GetObjectItem(m, "role");
                        cJSON *text = cJSON_GetObjectItem(m, "text");
                        if (role && text && cJSON_IsString(role) && cJSON_IsString(text))
                            storage_add_message(c, role->valuestring, text->valuestring);
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
    return h;
}

int storage_save(History *h) {
    if (!h) return 0;
    ensure_dir();
    char dir[512], path[600], tmp[620];
    storage_dir(dir, sizeof(dir));
    snprintf(path, sizeof(path), "%s/history.json", dir);
    snprintf(tmp, sizeof(tmp), "%s/history.json.tmp", dir);

    cJSON *root = cJSON_CreateObject();
    if (!root) return 0;
    cJSON_AddNumberToObject(root, "next_id", h->next_id);
    cJSON *chats = cJSON_AddObjectToObject(root, "chats");
    cJSON *messages = cJSON_AddObjectToObject(root, "messages");

    for (int i = 0; i < h->chat_count; i++) {
        Chat *c = &h->chats[i];
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddStringToObject(entry, "name", c->name ? c->name : "Chat");
        cJSON_AddStringToObject(entry, "time", c->time ? c->time : "");
        cJSON_AddItemToObject(chats, c->id, entry);

        cJSON *arr = cJSON_AddArrayToObject(messages, c->id);
        for (int j = 0; j < c->msg_count; j++) {
            cJSON *m = cJSON_CreateObject();
            cJSON_AddStringToObject(m, "role", c->messages[j].role ? c->messages[j].role : "user");
            cJSON_AddStringToObject(m, "text", c->messages[j].text ? c->messages[j].text : "");
            cJSON_AddStringToObject(m, "time", c->messages[j].time ? c->messages[j].time : "");
            cJSON_AddItemToArray(arr, m);
        }
    }

    char *out = cJSON_Print(root);
    cJSON_Delete(root);
    if (!out) return 0;

    FILE *f = fopen(tmp, "w");
    if (!f) { free(out); return 0; }
    fwrite(out, 1, strlen(out), f);
    fclose(f);
    free(out);
    return rename(tmp, path) == 0;
}

void storage_free(History *h) {
    if (!h) return;
    for (int i = 0; i < h->chat_count; i++) free_chat_contents(&h->chats[i]);
    free(h->chats);
    free(h);
}

/* --------------------------------------------------------------------- */
/*  Import / export                                                       */
/* --------------------------------------------------------------------- */
int storage_export_chat(Chat *c, const char *path) {
    if (!c || !path) return 0;
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "# %s\n", c->name ? c->name : "Chat");
    for (int i = 0; i < c->msg_count; i++) {
        fprintf(f, "\n[%s] %s: %s\n",
                c->messages[i].time ? c->messages[i].time : "",
                c->messages[i].role ? c->messages[i].role : "user",
                c->messages[i].text ? c->messages[i].text : "");
    }
    fclose(f);
    return 1;
}

Chat* storage_import_chat(History *h, const char *path) {
    if (!h || !path) return NULL;
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    char name[128];
    char *base = strrchr(path, '/');
    snprintf(name, sizeof(name), "%s", base ? base + 1 : path);

    char *slug = slugify(name);
    Chat *c = storage_create_chat(h, slug);
    free(slug);
    if (!c) { fclose(f); return NULL; }

    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (!line[0] || line[0] == '#') continue;
        char *colon = strchr(line, ':');
        if (colon) {
            char role[64] = "user";
            if (strstr(line, "NEON") || strstr(line, "assistant")) snprintf(role, sizeof(role), "assistant");
            else if (strstr(line, "System")) snprintf(role, sizeof(role), "system");
            storage_add_message(c, role, colon + 1);
        } else {
            storage_add_message(c, "user", line);
        }
    }
    fclose(f);
    return c;
}
