#include "settings.h"
#include <sys/stat.h>

/* ===================================================================== */
/*  Paths                                                                 */
/* ===================================================================== */
static void app_dir(char *out, size_t n) {
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(out, n, "%s/.neon_core_ai", home);
}

static void ensure_app_dir(void) {
    char dir[512];
    app_dir(dir, sizeof(dir));
    mkdir(dir, 0700);
}

/* ===================================================================== */
/*  Settings persistence                                                  */
/* ===================================================================== */
static void settings_defaults(Settings *s) {
    memset(s, 0, sizeof(*s));
    snprintf(s->theme, sizeof(s->theme), "Neon Blue");
    s->font_size = 12;
    snprintf(s->ascii_style, sizeof(s->ascii_style), "block");
    s->ascii_width = 50;
    snprintf(s->personality, sizeof(s->personality), "friendly");
    snprintf(s->response_style, sizeof(s->response_style), "detailed");
    s->creative_mode = 1;
    s->auto_save = 1;
    s->show_timestamps = 0;
    s->enable_reminders = 1;
    s->auto_clear = 0;
    s->humor_level = 50;
    s->verbosity = 50;
    s->voice_rate = 175;
    s->voice_pitch = 50;
}

Settings* settings_load(void) {
    Settings *s = (Settings *)calloc(1, sizeof(Settings));
    if (!s) return NULL;
    settings_defaults(s);

    ensure_app_dir();
    char dir[512], path[600];
    app_dir(dir, sizeof(dir));
    snprintf(path, sizeof(path), "%s/settings.json", dir);

    FILE *f = fopen(path, "r");
    if (!f) return s;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || size > 1024 * 1024) { fclose(f); return s; }
    char *buf = (char *)calloc((size_t)size + 1, sizeof(char));
    if (!buf) { fclose(f); return s; }
    size_t rd = fread(buf, 1, (size_t)size, f);
    buf[rd] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    if (root) {
        cJSON *it;
#define GETS(key, field) do { it = cJSON_GetObjectItem(root, key); \
        if (it && cJSON_IsString(it) && it->valuestring) \
            snprintf(s->field, sizeof(s->field), "%s", it->valuestring); } while (0)
#define GETI(key, field) do { it = cJSON_GetObjectItem(root, key); \
        if (it && cJSON_IsNumber(it)) s->field = it->valueint; } while (0)
        GETS("theme", theme);
        GETI("font_size", font_size);
        GETS("ascii_style", ascii_style);
        GETI("ascii_width", ascii_width);
        GETS("personality", personality);
        GETS("response_style", response_style);
        GETI("creative_mode", creative_mode);
        GETI("auto_save", auto_save);
        GETI("show_timestamps", show_timestamps);
        GETI("enable_reminders", enable_reminders);
        GETI("auto_clear", auto_clear);
        GETI("humor_level", humor_level);
        GETI("verbosity", verbosity);
        GETI("voice_rate", voice_rate);
        GETI("voice_pitch", voice_pitch);
#undef GETS
#undef GETI
        cJSON_Delete(root);
    }
    free(buf);
    return s;
}

int settings_save(Settings *s) {
    if (!s) return 0;
    ensure_app_dir();
    char dir[512], path[600], tmp[620];
    app_dir(dir, sizeof(dir));
    snprintf(path, sizeof(path), "%s/settings.json", dir);
    snprintf(tmp, sizeof(tmp), "%s/settings.json.tmp", dir);

    cJSON *root = cJSON_CreateObject();
    if (!root) return 0;
    cJSON_AddStringToObject(root, "theme", s->theme);
    cJSON_AddNumberToObject(root, "font_size", s->font_size);
    cJSON_AddStringToObject(root, "ascii_style", s->ascii_style);
    cJSON_AddNumberToObject(root, "ascii_width", s->ascii_width);
    cJSON_AddStringToObject(root, "personality", s->personality);
    cJSON_AddStringToObject(root, "response_style", s->response_style);
    cJSON_AddBoolToObject(root, "creative_mode", s->creative_mode);
    cJSON_AddBoolToObject(root, "auto_save", s->auto_save);
    cJSON_AddBoolToObject(root, "show_timestamps", s->show_timestamps);
    cJSON_AddBoolToObject(root, "enable_reminders", s->enable_reminders);
    cJSON_AddBoolToObject(root, "auto_clear", s->auto_clear);
    cJSON_AddNumberToObject(root, "humor_level", s->humor_level);
    cJSON_AddNumberToObject(root, "verbosity", s->verbosity);
    cJSON_AddNumberToObject(root, "voice_rate", s->voice_rate);
    cJSON_AddNumberToObject(root, "voice_pitch", s->voice_pitch);

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

/* ===================================================================== */
/*  Themes                                                                */
/* ===================================================================== */
Theme* settings_load_themes(const char *path, int *count) {
    *count = 0;
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || size > 1024 * 1024) { fclose(f); return NULL; }
    char *buf = (char *)calloc((size_t)size + 1, sizeof(char));
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)size, f);
    buf[rd] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return NULL;

    cJSON *arr = cJSON_GetObjectItem(root, "themes");
    int n = arr ? cJSON_GetArraySize(arr) : 0;
    if (n <= 0) { cJSON_Delete(root); return NULL; }

    Theme *themes = (Theme *)calloc((size_t)n, sizeof(Theme));
    if (!themes) { cJSON_Delete(root); return NULL; }

    int out = 0;
    for (int i = 0; i < n; i++) {
        cJSON *t = cJSON_GetArrayItem(arr, i);
        if (!t) continue;
        cJSON *v;
#define COPY(field, key) do { v = cJSON_GetObjectItem(t, key); \
        if (v && cJSON_IsString(v) && v->valuestring) \
            snprintf(themes[out].field, sizeof(themes[out].field), "%s", v->valuestring); } while (0)
        COPY(name, "name");
        COPY(bg, "bg");
        COPY(fg, "fg");
        COPY(bg2, "bg2");
        COPY(accent, "accent");
#undef COPY
        if (themes[out].name[0]) out++;
    }
    cJSON_Delete(root);
    *count = out;
    return themes;
}

void settings_free_themes(Theme *themes, int count) {
    (void)count;
    free(themes);
}

const Theme* settings_find_theme(const Theme *themes, int count, const char *name) {
    if (!themes || count <= 0) return NULL;
    for (int i = 0; i < count; i++)
        if (name && strcmp(themes[i].name, name) == 0) return &themes[i];
    return &themes[0];
}

/* ===================================================================== */
/*  Theme application                                                     */
/* ===================================================================== */
static GtkCssProvider *g_css = NULL;

void settings_apply_theme(GtkWidget *window, const Theme *t) {
    if (!t) return;
    GdkScreen *screen = gtk_widget_get_screen(window ? window : gtk_window_new(GTK_WINDOW_TOPLEVEL));

    if (!g_css) {
        g_css = gtk_css_provider_new();
        gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(g_css),
                                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    char *css = g_strdup_printf(
        "window, .background { background-color: %s; color: %s; }"
        "headerbar { background-color: %s; color: %s; }"
        "textview, textview text { background-color: %s; color: %s; }"
        "entry { background-color: %s; color: %s; }"
        "button { background-image: none; background-color: %s; color: %s; "
        "  border-color: %s; }"
        "button:hover { background-color: %s; }"
        "list { background-color: %s; color: %s; }"
        "list row:selected { background-color: %s; }"
        "label { color: %s; }"
        "statusbar { background-color: %s; color: %s; }",
        t->bg, t->fg,
        t->bg2, t->accent,
        t->bg, t->fg,
        t->bg2, t->fg,
        t->bg2, t->fg, t->accent,
        t->accent,
        t->bg2, t->fg,
        t->accent,
        t->fg,
        t->bg2, t->fg);

    gtk_css_provider_load_from_data(g_css, css, -1, NULL);
    g_free(css);
}

/* ===================================================================== */
/*  Settings dialog                                                       */
/* ===================================================================== */
static const char* ASCII_STYLES[] = {"block", "shade", "diagonal", "dots", "numbers", NULL};
static const char* PERSONALITIES[] = {"friendly", "witty", "sarcastic", "professional", "philosophical", NULL};
static const char* RESPONSE_STYLES[] = {"concise", "detailed", "comprehensive", NULL};

static GtkWidget* add_row(GtkWidget *grid, int row, const char *label_text, GtkWidget *widget) {
    GtkWidget *lbl = gtk_label_new(label_text);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
    return widget;
}

typedef struct {
    GtkWidget *dialog;
    Settings  *s;
    GtkWidget *theme_combo;
    GtkWidget *font_spin;
    GtkWidget *ascii_combo;
    GtkWidget *ascii_width;
    GtkWidget *personality_combo;
    GtkWidget *response_combo;
    GtkWidget *creative_check;
    GtkWidget *autosave_check;
    GtkWidget *timestamps_check;
    GtkWidget *reminders_check;
    GtkWidget *autoclear_check;
    GtkWidget *humor_scale;
    GtkWidget *verbosity_scale;
    GtkWidget *rate_scale;
    GtkWidget *pitch_scale;
} DialogRefs;

static void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer data) {
    DialogRefs *refs = (DialogRefs *)data;
    if (response_id != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(GTK_WIDGET(dialog));
        return;
    }
    Settings *s = refs->s;

    const char *theme = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(refs->theme_combo));
    if (theme) snprintf(s->theme, sizeof(s->theme), "%s", theme);
    s->font_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(refs->font_spin));
    const char *style = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(refs->ascii_combo));
    if (style) snprintf(s->ascii_style, sizeof(s->ascii_style), "%s", style);
    s->ascii_width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(refs->ascii_width));
    const char *pers = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(refs->personality_combo));
    if (pers) snprintf(s->personality, sizeof(s->personality), "%s", pers);
    const char *rs = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(refs->response_combo));
    if (rs) snprintf(s->response_style, sizeof(s->response_style), "%s", rs);

    s->creative_mode    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(refs->creative_check));
    s->auto_save        = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(refs->autosave_check));
    s->show_timestamps  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(refs->timestamps_check));
    s->enable_reminders = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(refs->reminders_check));
    s->auto_clear       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(refs->autoclear_check));

    s->humor_level = (int)gtk_range_get_value(GTK_RANGE(refs->humor_scale));
    s->verbosity   = (int)gtk_range_get_value(GTK_RANGE(refs->verbosity_scale));
    s->voice_rate  = (int)gtk_range_get_value(GTK_RANGE(refs->rate_scale));
    s->voice_pitch = (int)gtk_range_get_value(GTK_RANGE(refs->pitch_scale));

    settings_save(s);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

int settings_show_dialog(GtkWindow *parent, Settings *s) {
    DialogRefs refs;
    memset(&refs, 0, sizeof(refs));
    refs.s = s;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "NEON CORE Settings", parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT, NULL);
    refs.dialog = dialog;
    gtk_window_set_default_size(GTK_WINDOW(dialog), 460, 620);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
    gtk_container_add(GTK_CONTAINER(content), grid);

    int row = 0;

    refs.theme_combo = gtk_combo_box_text_new();
    /* populate themes from assets/themes.json */
    int tcount = 0;
    Theme *themes = settings_load_themes("assets/themes.json", &tcount);
    for (int i = 0; i < tcount; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refs.theme_combo), themes[i].name);
    settings_free_themes(themes, tcount);
    if (tcount == 0) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refs.theme_combo), "Neon Blue");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refs.theme_combo), "Matrix");
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(refs.theme_combo), 0);
    add_row(grid, row++, "Theme", refs.theme_combo);

    refs.font_spin = gtk_spin_button_new_with_range(8, 24, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(refs.font_spin), s->font_size);
    add_row(grid, row++, "Font size", refs.font_spin);

    refs.ascii_combo = gtk_combo_box_text_new();
    for (int i = 0; ASCII_STYLES[i]; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refs.ascii_combo), ASCII_STYLES[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(refs.ascii_combo), 0);
    add_row(grid, row++, "ASCII style", refs.ascii_combo);

    refs.ascii_width = gtk_spin_button_new_with_range(20, 200, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(refs.ascii_width), s->ascii_width);
    add_row(grid, row++, "ASCII width", refs.ascii_width);

    refs.personality_combo = gtk_combo_box_text_new();
    for (int i = 0; PERSONALITIES[i]; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refs.personality_combo), PERSONALITIES[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(refs.personality_combo), 0);
    add_row(grid, row++, "Personality", refs.personality_combo);

    refs.response_combo = gtk_combo_box_text_new();
    for (int i = 0; RESPONSE_STYLES[i]; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refs.response_combo), RESPONSE_STYLES[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(refs.response_combo), 1);
    add_row(grid, row++, "Response style", refs.response_combo);

    refs.creative_check   = gtk_check_button_new_with_label("Creative mode");
    refs.autosave_check   = gtk_check_button_new_with_label("Auto-save chats");
    refs.timestamps_check = gtk_check_button_new_with_label("Show timestamps");
    refs.reminders_check  = gtk_check_button_new_with_label("Enable reminders");
    refs.autoclear_check  = gtk_check_button_new_with_label("Auto-clear on exit");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(refs.creative_check), s->creative_mode);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(refs.autosave_check), s->auto_save);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(refs.timestamps_check), s->show_timestamps);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(refs.reminders_check), s->enable_reminders);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(refs.autoclear_check), s->auto_clear);
    gtk_grid_attach(GTK_GRID(grid), refs.creative_check, 0, row++, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), refs.autosave_check, 0, row++, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), refs.timestamps_check, 0, row++, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), refs.reminders_check, 0, row++, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), refs.autoclear_check, 0, row++, 2, 1);

    refs.humor_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_range_set_value(GTK_RANGE(refs.humor_scale), s->humor_level);
    add_row(grid, row++, "Humor level", refs.humor_scale);

    refs.verbosity_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_range_set_value(GTK_RANGE(refs.verbosity_scale), s->verbosity);
    add_row(grid, row++, "Verbosity", refs.verbosity_scale);

    refs.rate_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 80, 400, 1);
    gtk_range_set_value(GTK_RANGE(refs.rate_scale), s->voice_rate);
    add_row(grid, row++, "Voice rate", refs.rate_scale);

    refs.pitch_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 99, 1);
    gtk_range_set_value(GTK_RANGE(refs.pitch_scale), s->voice_pitch);
    add_row(grid, row++, "Voice pitch", refs.pitch_scale);

    g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), &refs);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    return 1;
}
