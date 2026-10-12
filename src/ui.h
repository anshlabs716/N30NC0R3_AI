#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"

typedef struct {
    GtkWidget    *window;
    GtkWidget    *chat_view;
    GtkTextBuffer*chat_buffer;
    GtkWidget    *input_entry;
    GtkWidget    *send_button;
    GtkWidget    *read_button;
    GtkWidget    *status_bar;
    GtkWidget    *session_list;
    GtkWidget    *ascii_width_spin;
    GtkWidget    *font_scale;

    Settings     *settings;
    int           status_ctx;

    char          last_assistant[16384];
    int           busy;
} UI;

extern UI *app_ui;

/* Build the main window and show it. */
void ui_init(Settings *settings);

/* Append a chat message. role is "user", "assistant" or "system". */
void ui_append_message(const char *role, const char *text);

/* Append a message whose text may contain §0-§6 colour markers. */
void ui_append_ascii(const char *text);

/* Status bar helper. */
void ui_set_status(const char *status);

/* Clear the chat view. */
void ui_clear_chat(void);

/* Apply a named theme (loaded from assets/themes.json). */
void ui_apply_theme(const char *theme_name);

#ifdef __cplusplus
}
#endif

#endif
