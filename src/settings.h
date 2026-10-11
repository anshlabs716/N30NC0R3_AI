#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "cJSON.h"

/* A single colour theme loaded from assets/themes.json. */
typedef struct {
    char name[64];
    char bg[16];
    char fg[16];
    char bg2[16];
    char accent[16];
} Theme;

/* Persisted application settings. */
typedef struct {
    char theme[64];
    int  font_size;
    char ascii_style[32];
    int  ascii_width;
    char personality[32];
    char response_style[32];
    int  creative_mode;
    int  auto_save;
    int  show_timestamps;
    int  enable_reminders;
    int  auto_clear;
    int  humor_level;
    int  verbosity;
    int  voice_rate;
    int  voice_pitch;
} Settings;

/* Load/save ~/.neoncore/settings.json. */
Settings* settings_load(void);
int       settings_save(Settings *s);

/* Load themes from assets/themes.json. Returns array; caller frees each name
 * via settings_free_themes(). *count receives the number of themes. */
Theme*    settings_load_themes(const char *path, int *count);
void      settings_free_themes(Theme *themes, int count);

/* Apply a theme to a widget tree (window + chat colours). */
void      settings_apply_theme(GtkWidget *window, const Theme *t);

/* Resolve a theme by name; falls back to the first theme. */
const Theme* settings_find_theme(const Theme *themes, int count, const char *name);

/* Show the modal settings dialog. Returns 1 if the user pressed Save. */
int       settings_show_dialog(GtkWindow *parent, Settings *s);

#ifdef __cplusplus
}
#endif

#endif
