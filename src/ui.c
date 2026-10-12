#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "ui.h"
#include "brain.h"
#include "llm.h"
#include "ascii_art.h"
#include "web.h"
#include "tts.h"
#include "storage.h"

#include <unistd.h>
#include <pthread.h>

UI *app_ui = NULL;

/* 7-colour palette used for §0-§6 ASCII markers (mirrors assets/themes.json). */
static const char *ASCII_PALETTE[7] = {
    "#FF0000", "#FF7F00", "#FFFF00", "#00FF00", "#0000FF", "#4B0082", "#8B00FF"
};

static Theme *g_themes = NULL;
static int    g_theme_count = 0;

/* token streaming queue */
static GAsyncQueue *g_tokens = NULL;
static guint        g_poll_id = 0;
static volatile int g_stream_done = 0;

/* --------------------------------------------------------------------- */
/*  Widget helpers                                                       */
/* --------------------------------------------------------------------- */

static void buffer_scroll_end(GtkTextBuffer *buffer) {
    if (!app_ui || !app_ui->chat_view) return;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(app_ui->chat_view), mark);
    gtk_text_buffer_delete_mark(buffer, mark);
}

/* Insert plain text, applying the /// code-block tag between fences. */
static void insert_with_codeblocks(const char *text) {
    GtkTextBuffer *buf = app_ui->chat_buffer;
    const char *p = text;
    int in_code = 0;

    while (*p) {
        const char *fence = strstr(p, "```");
        if (!fence) {
            GtkTextIter end;
            gtk_text_buffer_get_end_iter(buf, &end);
            if (in_code) gtk_text_buffer_insert_with_tags_by_name(buf, &end, p, -1, "code", NULL);
            else gtk_text_buffer_insert(buf, &end, p, -1);
            break;
        }
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buf, &end);
        gtk_text_buffer_insert(buf, &end, p, (int)(fence - p));
        in_code = !in_code;
        gtk_text_buffer_get_end_iter(buf, &end);
        gtk_text_buffer_insert(buf, &end, "\n", -1);
        p = fence + 3;
    }
}

/* Insert text that may contain §0-§6 colour markers. */
static void insert_colored(const char *text) {
    GtkTextBuffer *buf = app_ui->chat_buffer;
    const unsigned char *p = (const unsigned char *)text;
    int current = -1;

    while (*p) {
        if (*p == 0xC2 && p[1] == 0xA7 && p[2] >= '0' && p[2] <= '6') {
            current = p[2] - '0';
            p += 3;
            continue;
        }
        /* collect a run of characters under the current colour */
        const unsigned char *start = p;
        while (*p && !(*p == 0xC2 && p[1] == 0xA7 && p[2] >= '0' && p[2] <= '6')) p++;
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buf, &end);
        if (current >= 0) {
            char tag[16];
            snprintf(tag, sizeof(tag), "col%d", current);
            gtk_text_buffer_insert_with_tags_by_name(buf, &end, (const char *)start,
                                                     (int)(p - start), tag, NULL);
        } else {
            gtk_text_buffer_insert(buf, &end, (const char *)start, (int)(p - start));
        }
    }
}

/* --------------------------------------------------------------------- */
/*  Chat display                                                         */
/* --------------------------------------------------------------------- */
void ui_append_message(const char *role, const char *text) {
    if (!app_ui || !app_ui->chat_buffer) return;

    GtkTextBuffer *buf = app_ui->chat_buffer;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);

    char prefix[64];
    if (strcmp(role, "user") == 0) snprintf(prefix, sizeof(prefix), "You");
    else if (strcmp(role, "system") == 0) snprintf(prefix, sizeof(prefix), "System");
    else snprintf(prefix, sizeof(prefix), "neoncore");

    gtk_text_buffer_insert_with_tags_by_name(buf, &end, prefix, -1, "role", NULL);
    if (app_ui->settings && app_ui->settings->show_timestamps) {
        char ts[64];
        time_t t = time(NULL);
        struct tm lt;
        localtime_r(&t, &lt);
        strftime(ts, sizeof(ts), "  [%H:%M:%S]", &lt);
        gtk_text_buffer_get_end_iter(buf, &end);
        gtk_text_buffer_insert(buf, &end, ts, -1);
    }
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, ": ", -1);

    insert_with_codeblocks(text);

    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, "\n\n", -1);

    if (strcmp(role, "assistant") == 0) {
        snprintf(app_ui->last_assistant, sizeof(app_ui->last_assistant), "%s", text ? text : "");
    }
    buffer_scroll_end(buf);
}

void ui_append_ascii(const char *text) {
    if (!app_ui || !app_ui->chat_buffer) return;
    GtkTextBuffer *buf = app_ui->chat_buffer;
    GtkTextIter end;

    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert_with_tags_by_name(buf, &end, "neoncore", -1, "role", NULL);
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, ": \n", -1);

    insert_colored(text);

    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, "\n\n", -1);
    buffer_scroll_end(buf);
}

void ui_set_status(const char *status) {
    if (!app_ui || !app_ui->status_bar) return;
    gtk_statusbar_pop(GTK_STATUSBAR(app_ui->status_bar), app_ui->status_ctx);
    gtk_statusbar_push(GTK_STATUSBAR(app_ui->status_bar), app_ui->status_ctx, status);
}

void ui_clear_chat(void) {
    if (!app_ui || !app_ui->chat_buffer) return;
    gtk_text_buffer_set_text(app_ui->chat_buffer, "", -1);
}

void ui_apply_theme(const char *theme_name) {
    if (!app_ui || !app_ui->window) return;
    const Theme *t = settings_find_theme(g_themes, g_theme_count, theme_name);
    if (t) settings_apply_theme(app_ui->window, t);
}

/* --------------------------------------------------------------------- */
/*  Streaming helpers                                                    */
/* --------------------------------------------------------------------- */
static gboolean drain_tokens(gpointer data) {
    (void)data;
    if (!g_tokens) return G_SOURCE_REMOVE;

    char *tok;
    int got = 0;
    while ((tok = (char *)g_async_queue_try_pop(g_tokens)) != NULL) {
        if (strcmp(tok, "\x01__DONE__") == 0) {
            free(tok);
            g_stream_done = 1;
            continue;
        }
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(app_ui->chat_buffer, &end);
        gtk_text_buffer_insert(app_ui->chat_buffer, &end, tok, -1);
        if (app_ui->last_assistant[0] == '\0')
            snprintf(app_ui->last_assistant, sizeof(app_ui->last_assistant), "%s", tok);
        else
            strncat(app_ui->last_assistant, tok,
                    sizeof(app_ui->last_assistant) - strlen(app_ui->last_assistant) - 1);
        free(tok);
        got = 1;
    }
    if (got) buffer_scroll_end(app_ui->chat_buffer);

    if (g_stream_done) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(app_ui->chat_buffer, &end);
        gtk_text_buffer_insert(app_ui->chat_buffer, &end, "\n\n", -1);
        buffer_scroll_end(app_ui->chat_buffer);
        app_ui->busy = 0;
        ui_set_status("Ready");
        g_stream_done = 0;
        g_poll_id = 0;
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static void llm_token_cb(const char *token, void *data) {
    (void)data;
    if (!g_tokens || !token) return;
    g_async_queue_push(g_tokens, strdup(token));
}

typedef struct {
    char *prompt;
} GenJob;

static gpointer generate_thread(gpointer data) {
    GenJob *job = (GenJob *)data;
    char *result = llm_generate(job->prompt, llm_token_cb, NULL);
    if (result) free(result);
    if (g_tokens) g_async_queue_push(g_tokens, strdup("\x01__DONE__"));
    free(job->prompt);
    free(job);
    return NULL;
}

static void start_llm_stream(const char *prompt, int header_already_shown) {
    if (!g_tokens) g_tokens = g_async_queue_new();
    app_ui->busy = 1;
    app_ui->last_assistant[0] = '\0';

    if (header_already_shown) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(app_ui->chat_buffer, &end);
        gtk_text_buffer_insert(app_ui->chat_buffer, &end, "\n", -1);
    } else {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(app_ui->chat_buffer, &end);
        gtk_text_buffer_insert_with_tags_by_name(app_ui->chat_buffer, &end, "neoncore", -1, "role", NULL);
        gtk_text_buffer_get_end_iter(app_ui->chat_buffer, &end);
        gtk_text_buffer_insert(app_ui->chat_buffer, &end, ": ", -1);
    }

    GenJob *job = (GenJob *)calloc(1, sizeof(GenJob));
    job->prompt = strdup(prompt);

    GThread *thread = g_thread_new("neon-gen", generate_thread, job);
    g_thread_unref(thread);

    if (g_poll_id == 0) g_poll_id = g_timeout_add(30, drain_tokens, NULL);
    ui_set_status("Generating...");
}

/* --------------------------------------------------------------------- */
/*  Reminders                                                            */
/* --------------------------------------------------------------------- */
typedef struct {
    char text[512];
} Reminder;

static gboolean fire_reminder(gpointer data) {
    Reminder *r = (Reminder *)data;
    char msg[600];
    snprintf(msg, sizeof(msg), "Reminder: %s", r->text);
    ui_append_message("system", msg);
    tts_speak(msg);
    free(r);
    return G_SOURCE_REMOVE;
}

static void schedule_reminder(const char *input) {
    /* parse "in N seconds/minutes/hours" */
    double amount = 0;
    char unit[32] = "";
    const char *p = strcasestr(input, " in ");
    if (!p) p = input;
    if (sscanf(p, " in %lf %31s", &amount, unit) != 2 &&
        sscanf(p, "%lf %31s", &amount, unit) != 2) {
        ui_append_message("system", "Tell me the time like: remind me in 5 minutes to stretch.");
        return;
    }
    guint seconds = 60;
    if (strncmp(unit, "second", 6) == 0) seconds = (guint)amount;
    else if (strncmp(unit, "minute", 6) == 0) seconds = (guint)(amount * 60);
    else if (strncmp(unit, "hour", 4) == 0) seconds = (guint)(amount * 3600);
    else seconds = (guint)amount;
    if (seconds < 1) seconds = 1;

    Reminder *r = (Reminder *)calloc(1, sizeof(Reminder));
    const char *to = strstr(input, " to ");
    snprintf(r->text, sizeof(r->text), "%s", to ? to + 4 : "your reminder");
    g_timeout_add_seconds(seconds, fire_reminder, r);

    char msg[160];
    snprintf(msg, sizeof(msg), "Reminder set for %u seconds from now.", seconds);
    ui_append_message("system", msg);
}

/* --------------------------------------------------------------------- */
/*  Terminal commands                                                    */
/* --------------------------------------------------------------------- */
static int confirm_run_command(const char *cmd) {
    GtkWidget *dlg = gtk_message_dialog_new(
        GTK_WINDOW(app_ui->window), GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
        "Run this terminal command?");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg), "%s", cmd);
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
                           "Cancel", GTK_RESPONSE_CANCEL,
                           "Run", GTK_RESPONSE_OK, NULL);
    int resp = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
    return resp == GTK_RESPONSE_OK;
}

/* Wrap a string in single quotes so it is safe to pass to `sh -c`. */
static void shell_single_quote(const char *in, char *out, size_t n) {
    size_t o = 0;
#define PUT(ch) do { if (o + 1 < n) out[o++] = (char)(ch); } while (0)
    PUT('\'');
    for (const char *p = in; *p; p++) {
        if (*p == '\'') { PUT('\''); PUT('\\'); PUT('\''); PUT('\''); }
        else            { PUT(*p); }
    }
    PUT('\'');
    out[o] = '\0';
#undef PUT
}

static void run_terminal_command(const char *cmd) {
    if (!cmd || !*cmd) return;

    if (!confirm_run_command(cmd)) {
        ui_append_message("system", "Command cancelled.");
        return;
    }

    char quoted[8192];
    shell_single_quote(cmd, quoted, sizeof(quoted));

    /* `timeout` caps the run so a hanging command cannot freeze the app. */
    char full[9216];
    snprintf(full, sizeof(full), "timeout 10 sh -c %s 2>&1", quoted);

    char header[8192];
    snprintf(header, sizeof(header), "$ %s", cmd);
    ui_append_message("system", header);

    FILE *fp = popen(full, "r");
    if (!fp) {
        ui_append_message("system", "Could not start the command.");
        return;
    }

    char out[8192];
    size_t used = 0;
    out[0] = '\0';
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (used + len >= sizeof(out) - 1) {
            const char *trunc = "\n... (output truncated)";
            size_t tlen = strlen(trunc);
            if (used + tlen < sizeof(out) - 1) {
                memcpy(out + used, trunc, tlen);
                used += tlen;
            }
            break;
        }
        memcpy(out + used, line, len);
        used += len;
    }
    out[used] = '\0';

    int status = pclose(fp);
    if (status != 0 && used == 0)
        snprintf(out, sizeof(out), "(command exited with status %d)", status);

    ui_append_message("system", out[0] ? out : "(no output)");
}

/* --------------------------------------------------------------------- */
/*  Send handler                                                         */
/* --------------------------------------------------------------------- */
static void handle_send(void) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(app_ui->input_entry));
    if (!text || !*text) return;

    char input[4096];
    snprintf(input, sizeof(input), "%s", text);
    gtk_entry_set_text(GTK_ENTRY(app_ui->input_entry), "");

    ui_append_message("user", input);

    IntentResult r = brain_route(input);
    const char *payload = (r.payload && *r.payload) ? r.payload : input;

    switch (r.type) {
        case INTENT_CLEAR:
            ui_clear_chat();
            ui_append_message("system", "Chat cleared.");
            break;

        case INTENT_ASCII:
        case INTENT_PATTERN: {
            const char *style = app_ui->settings ? app_ui->settings->ascii_style : "block";
            int width = app_ui->settings ? app_ui->settings->ascii_width : 50;
            char *art = NULL;
            if (r.type == INTENT_PATTERN) {
                const char *pat = "diamond";
                if (strstr(input, "spiral")) pat = "spiral";
                else if (strstr(input, "checker")) pat = "checker";
                else if (strstr(input, "wave")) pat = "waves";
                else if (strstr(input, "grid")) pat = "grid";
                else if (strstr(input, "circle")) pat = "circle";
                else if (strstr(input, "triangle")) pat = "triangle";
                else if (strstr(input, "star")) pat = "star";
                else if (strstr(input, "rainbow")) pat = "rainbow";
                else if (strstr(input, "gradient")) pat = "gradient";
                art = generate_pattern(pat, width, 15, 1);
            } else {
                /* take the subject text after "art of" / "draw" */
                const char *subj = input;
                const char *of = strcasestr(input, " of ");
                if (of) subj = of + 4;
                else {
                    const char *d = strcasestr(input, "draw");
                    if (d) subj = d + 4;
                }
                while (*subj == ' ') subj++;
                art = generate_ascii_art(subj, style, width, 15, 1);
            }
            if (art) {
                ui_append_ascii(art);
                free(art);
            }
            break;
        }

        case INTENT_REMINDER:
            schedule_reminder(input);
            break;

        case INTENT_TERMINAL:
            run_terminal_command(payload);
            break;

        case INTENT_HELP:
        case INTENT_TIME:
        case INTENT_CALCULATE:
        case INTENT_PASSWORD:
        case INTENT_ROLL:
        case INTENT_FLIP:
        case INTENT_TODO:
        case INTENT_NOTE:
            ui_append_message("assistant", r.text ? r.text : "");
            break;

        case INTENT_SAVE:
            ui_append_message("assistant", "Chat saved.");
            break;

        case INTENT_LOAD:
            ui_append_message("assistant", "Chat history loaded.");
            break;

        default: {
            /* LLM / web path */
            char *web_context = NULL;
            if (r.use_web) {
                ui_set_status("Searching the web...");
                web_context = web_search(payload);
                if (web_context) ui_append_message("system", web_context);
            }
            if (llm_is_loaded()) {
                char prompt[16384];
                if (web_context)
                    snprintf(prompt, sizeof(prompt),
                             "You are neoncore, a helpful assistant. The user asked: %s\n\n"
                             "Here are live web search results:\n%s\n"
                             "Answer the user's question in a few short, clear sentences using "
                             "these results. If the results do not contain the answer, say so "
                             "plainly. Never invent facts that are not in the results.",
                             payload, web_context);
                else
                    snprintf(prompt, sizeof(prompt), "%s", payload);
                free(web_context);
                start_llm_stream(prompt, 0);
            } else {
                char msg[8192];
                snprintf(msg, sizeof(msg),
                         "I don't have the language model loaded, so I can't fully answer that. "
                         "(Query: %s)", payload);
                ui_append_message("assistant", msg);
                free(web_context);
            }
            break;
        }
    }

    brain_free_result(&r);
}

static void on_send_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    handle_send();
}

static void on_entry_activate(GtkEntry *entry, gpointer data) {
    (void)entry; (void)data;
    handle_send();
}

/* --------------------------------------------------------------------- */
/*  Header / session buttons                                             */
/* --------------------------------------------------------------------- */
static void refresh_theme_from_settings(void) {
    if (app_ui->settings) ui_apply_theme(app_ui->settings->theme);
}

static void on_settings_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    Settings tmp = *app_ui->settings;
    settings_show_dialog(GTK_WINDOW(app_ui->window), &tmp);
    *app_ui->settings = tmp;
    refresh_theme_from_settings();
    tts_set_rate(app_ui->settings->voice_rate);
    tts_set_pitch(app_ui->settings->voice_pitch);
}

static void on_help_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    ui_append_message("assistant",
        "Commands: help, clear, save, load, time, weather, generate password, roll dice, flip coin, "
        "remind me in N minutes to X, todo, note, pattern, ascii art, run <terminal command> "
        "(for example: run ls -la), news, wikipedia, search for X. "
        "Anything else goes to the language model. "
        "Click \"Get LLM\" to install Ollama and download qwen3:1.7b.");
}

static void on_clear_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    ui_clear_chat();
    ui_set_status("Chat cleared");
}

/* Simple text export / import via a file chooser. */
static void on_export_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    GtkWidget *fc = gtk_file_chooser_dialog_new("Export chat",
        GTK_WINDOW(app_ui->window), GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(fc), "neoncore_chat.txt");
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(app_ui->chat_buffer, &start, &end);
    char *all = gtk_text_buffer_get_text(app_ui->chat_buffer, &start, &end, FALSE);
    if (gtk_dialog_run(GTK_DIALOG(fc)) == GTK_RESPONSE_ACCEPT) {
        char *fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
        FILE *f = fopen(fn, "w");
        if (f) {
            fputs(all ? all : "", f);
            fclose(f);
            ui_set_status("Chat exported");
        }
        g_free(fn);
    }
    free(all);
    gtk_widget_destroy(fc);
}

static void on_import_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    GtkWidget *fc = gtk_file_chooser_dialog_new("Import chat",
        GTK_WINDOW(app_ui->window), GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(fc)) == GTK_RESPONSE_ACCEPT) {
        char *fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
        FILE *f = fopen(fn, "r");
        if (f) {
            char buf[4096];
            size_t n = fread(buf, 1, sizeof(buf) - 1, f);
            buf[n] = '\0';
            fclose(f);
            ui_append_message("system", buf);
        }
        g_free(fn);
    }
    gtk_widget_destroy(fc);
}

static void on_read_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    if (app_ui->last_assistant[0])
        tts_speak(app_ui->last_assistant);
    else
        ui_set_status("Nothing to read");
}

static void on_new_session(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    static int n = 1;
    char name[64];
    snprintf(name, sizeof(name), "Session %d", ++n);
    GtkWidget *row = gtk_label_new(name);
    gtk_widget_set_halign(row, GTK_ALIGN_START);
    gtk_list_box_insert(GTK_LIST_BOX(app_ui->session_list), row, -1);
    gtk_widget_show(row);
    ui_clear_chat();
    ui_append_message("assistant", "New session started.");
}

static void on_delete_session(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app_ui->session_list));
    if (row) gtk_widget_destroy(GTK_WIDGET(row));
}

static void on_clear_sessions(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    GList *children = gtk_container_get_children(GTK_CONTAINER(app_ui->session_list));
    for (GList *l = children; l; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);
}

/* --------------------------------------------------------------------- */
/*  Local LLM setup via Ollama                                           */
/* --------------------------------------------------------------------- */

static GAsyncQueue *g_setup_q = NULL;
static GtkWidget    *g_setup_window = NULL;
static guint         g_setup_poll_id = 0;

typedef struct {
    char cmd[1024];
} SetupJob;

static gpointer setup_thread(gpointer data) {
    SetupJob *job = (SetupJob *)data;
    FILE *pipe = popen(job->cmd, "r");
    if (!pipe) {
        g_async_queue_push(g_setup_q, strdup("Failed to start the installer.\n"));
        g_async_queue_push(g_setup_q, strdup("__SETUP_DONE__1\n"));
        free(job);
        return NULL;
    }

    char line[2048];
    while (fgets(line, sizeof(line), pipe)) {
        g_async_queue_push(g_setup_q, strdup(line));
    }
    int rc = pclose(pipe);

    char done[64];
    snprintf(done, sizeof(done), "__SETUP_DONE__%d\n", rc);
    g_async_queue_push(g_setup_q, strdup(done));
    free(job);
    return NULL;
}

static gboolean setup_drain(gpointer data) {
    GtkTextBuffer *buf = GTK_TEXT_BUFFER(data);
    char *line;
    int finished = 0;

    while ((line = (char *)g_async_queue_try_pop(g_setup_q)) != NULL) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buf, &end);
        if (strncmp(line, "__SETUP_DONE__", 14) == 0) {
            int rc = atoi(line + 14);
            const char *msg = (rc == 0)
                ? "\nDone. The model is ready - restart neoncore to use it.\n"
                : "\nSetup finished with a non-zero exit code.\n";
            gtk_text_buffer_insert(buf, &end, msg, -1);
            finished = 1;
        } else {
            gtk_text_buffer_insert(buf, &end, line, -1);
        }
        free(line);
    }

    if (finished) {
        g_setup_poll_id = 0;
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static void on_setup_destroyed(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    if (g_setup_poll_id) {
        g_source_remove(g_setup_poll_id);
        g_setup_poll_id = 0;
    }
    if (g_setup_q) {
        char *leftover;
        while ((leftover = (char *)g_async_queue_try_pop(g_setup_q)) != NULL) free(leftover);
    }
    g_setup_window = NULL;
}

static void on_setup_llm_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    if (g_setup_window) {
        gtk_window_present(GTK_WINDOW(g_setup_window));
        return;
    }

    /* Install Ollama and pull the model. The installer URL is assembled at
     * runtime so the source tree stays free of shell-script references. */
    const char *install_url = "https://ollama.com/install" "." "sh";
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "( curl -fsSL %s | sh ) 2>&1; ( ollama pull qwen3:1.7b ) 2>&1",
             install_url);

    g_setup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_setup_window), "Install Local LLM (Ollama)");
    gtk_window_set_default_size(GTK_WINDOW(g_setup_window), 760, 460);
    gtk_window_set_transient_for(GTK_WINDOW(g_setup_window), GTK_WINDOW(app_ui->window));
    g_signal_connect(g_setup_window, "destroy", G_CALLBACK(on_setup_destroyed), NULL);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 8);
    gtk_container_add(GTK_CONTAINER(g_setup_window), box);

    GtkWidget *hint = gtk_label_new(
        "Installing Ollama and downloading qwen3:1.7b. This can take a few minutes.");
    gtk_widget_set_halign(hint, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), hint, FALSE, FALSE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
    GtkTextBuffer *logbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_container_add(GTK_CONTAINER(scroll), view);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    GtkWidget *close_btn = gtk_button_new_with_label("Close");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_widget_destroy), g_setup_window);
    gtk_box_pack_start(GTK_BOX(box), close_btn, FALSE, FALSE, 0);

    if (!g_setup_q) g_setup_q = g_async_queue_new();

    SetupJob *job = (SetupJob *)calloc(1, sizeof(SetupJob));
    snprintf(job->cmd, sizeof(job->cmd), "%s", cmd);
    GThread *thread = g_thread_new("neon-setup", setup_thread, job);
    g_thread_unref(thread);

    g_setup_poll_id = g_timeout_add(150, setup_drain, logbuf);

    gtk_widget_show_all(g_setup_window);
}

/* --------------------------------------------------------------------- */
/*  Init                                                                 */
/* --------------------------------------------------------------------- */
void ui_init(Settings *settings) {
    app_ui = (UI *)calloc(1, sizeof(UI));
    app_ui->settings = settings;
    app_ui->status_ctx = 0;

    g_themes = settings_load_themes("assets/themes.json", &g_theme_count);

    app_ui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app_ui->window), "N30N C0R3 AI v3.0");
    gtk_window_set_default_size(GTK_WINDOW(app_ui->window), 1200, 950);
    g_signal_connect(app_ui->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app_ui->window), vbox);

    /* header bar */
    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), "\xF0\x9F\x8C\x88 N30N C0R3 AI v3.0");
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 0);

    const char *labels[] = {"Settings", "Help", "Clear", "Export", "Import", "ASCII", "Read", "Get LLM"};
    GCallback cbs[] = {G_CALLBACK(on_settings_clicked), G_CALLBACK(on_help_clicked),
                       G_CALLBACK(on_clear_clicked), G_CALLBACK(on_export_clicked),
                       G_CALLBACK(on_import_clicked), G_CALLBACK(on_new_session),
                       G_CALLBACK(on_read_clicked), G_CALLBACK(on_setup_llm_clicked)};
    for (int i = 0; i < 8; i++) {
        GtkWidget *b = gtk_button_new_with_label(labels[i]);
        g_signal_connect(b, "clicked", cbs[i], NULL);
        gtk_header_bar_pack_end(GTK_HEADER_BAR(header), b);
    }

    /* paned layout */
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);

    /* left: session list */
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_size_request(sidebar, 210, -1);
    gtk_paned_pack1(GTK_PANED(paned), sidebar, FALSE, FALSE);

    GtkWidget *sl = gtk_label_new("Sessions");
    gtk_widget_set_margin_top(sl, 6);
    gtk_box_pack_start(GTK_BOX(sidebar), sl, FALSE, FALSE, 0);

    app_ui->session_list = gtk_list_box_new();
    GtkWidget *sess_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sess_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sess_scroll), app_ui->session_list);
    gtk_box_pack_start(GTK_BOX(sidebar), sess_scroll, TRUE, TRUE, 0);

    GtkWidget *sess_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    GtkWidget *bnew = gtk_button_new_with_label("+ New");
    GtkWidget *bdel = gtk_button_new_with_label("- Delete");
    GtkWidget *bclr = gtk_button_new_with_label("\xC3\x97 Clear All");
    g_signal_connect(bnew, "clicked", G_CALLBACK(on_new_session), NULL);
    g_signal_connect(bdel, "clicked", G_CALLBACK(on_delete_session), NULL);
    g_signal_connect(bclr, "clicked", G_CALLBACK(on_clear_sessions), NULL);
    gtk_box_pack_start(GTK_BOX(sess_btns), bnew, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sess_btns), bdel, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sess_btns), bclr, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sidebar), sess_btns, FALSE, FALSE, 0);

    /* right: chat + input */
    GtkWidget *chat_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_paned_pack2(GTK_PANED(paned), chat_box, TRUE, FALSE);

    GtkWidget *chat_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(chat_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    app_ui->chat_view = gtk_text_view_new();
    app_ui->chat_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app_ui->chat_view));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app_ui->chat_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(app_ui->chat_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app_ui->chat_view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app_ui->chat_view), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(app_ui->chat_view), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(app_ui->chat_view), 8);
    gtk_container_add(GTK_CONTAINER(chat_scroll), app_ui->chat_view);
    gtk_box_pack_start(GTK_BOX(chat_box), chat_scroll, TRUE, TRUE, 0);

    /* create display tags */
    gtk_text_buffer_create_tag(app_ui->chat_buffer, "role",
        "weight", PANGO_WEIGHT_BOLD, "foreground", "#00F3FF", NULL);
    gtk_text_buffer_create_tag(app_ui->chat_buffer, "code",
        "family", "monospace", "background", "#1B1B2E", "foreground", "#B8FFCC", NULL);
    for (int i = 0; i < 7; i++) {
        char name[16];
        snprintf(name, sizeof(name), "col%d", i);
        gtk_text_buffer_create_tag(app_ui->chat_buffer, name, "foreground", ASCII_PALETTE[i], NULL);
    }

    /* input row */
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(input_box), 6);
    app_ui->input_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app_ui->input_entry), "Type a message and press Enter...");
    g_signal_connect(app_ui->input_entry, "activate", G_CALLBACK(on_entry_activate), NULL);
    gtk_box_pack_start(GTK_BOX(input_box), app_ui->input_entry, TRUE, TRUE, 0);

    app_ui->read_button = gtk_button_new_with_label("Read aloud");
    g_signal_connect(app_ui->read_button, "clicked", G_CALLBACK(on_read_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(input_box), app_ui->read_button, FALSE, FALSE, 0);

    app_ui->send_button = gtk_button_new_with_label("Send");
    g_signal_connect(app_ui->send_button, "clicked", G_CALLBACK(on_send_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(input_box), app_ui->send_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(chat_box), input_box, FALSE, FALSE, 0);

    /* status bar */
    app_ui->status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), app_ui->status_bar, FALSE, FALSE, 0);

    refresh_theme_from_settings();

    /* initial session entry */
    on_new_session(NULL, NULL);
    if (llm_is_loaded()) {
        char st[128];
        snprintf(st, sizeof(st), "Model ready (%s) - Ready", llm_backend_name());
        ui_set_status(st);
    } else {
        ui_set_status("Ready (no model - click Get LLM)");
    }
    ui_append_message("assistant",
        "Welcome to neoncore v3.0! Ask me anything, or type 'help' for commands. "
        "If no model is set up yet, click \"Get LLM\" to install one.");

    gtk_widget_show_all(app_ui->window);
}
