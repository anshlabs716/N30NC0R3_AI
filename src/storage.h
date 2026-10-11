#ifndef STORAGE_H
#define STORAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cJSON.h"

#define STORAGE_MAX_MESSAGES 512

typedef struct {
    char *role;   /* "user" / "assistant" / "system" */
    char *text;
    char *time;
} Message;

typedef struct {
    char    *id;
    char    *name;
    char    *time;
    Message *messages;
    int      msg_count;
    int      msg_cap;
} Chat;

typedef struct {
    Chat *chats;
    int   chat_count;
    int   chat_cap;
    int   next_id;
} History;

/* Load / save ~/.neoncore/history.json (atomic write via rename). */
History* storage_load(void);
int      storage_save(History *h);
void     storage_free(History *h);

/* Chat lifecycle. Returns the new chat (owned by the History). */
Chat* storage_create_chat(History *h, const char *name);
int   storage_delete_chat(History *h, const char *id);
int   storage_rename_chat(History *h, const char *id, const char *name);
void  storage_clear_all(History *h);

/* Append a message to a chat. */
int   storage_add_message(Chat *c, const char *role, const char *text);

/* Export a chat to a plain-text file, or import a file into a new chat. */
int   storage_export_chat(Chat *c, const char *path);
Chat* storage_import_chat(History *h, const char *path);

/* The on-disk directory (~/.neoncore). */
void  storage_dir(char *out, size_t n);

#ifdef __cplusplus
}
#endif

#endif
