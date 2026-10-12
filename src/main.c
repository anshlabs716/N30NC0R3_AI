#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include "ui.h"
#include "brain.h"
#include "llm.h"
#include "tts.h"
#include "settings.h"

/*
 * NEON CORE AI v3.0
 * -----------------
 * Entry point. Initialises GTK, loads settings and the local language
 * model, then hands control to the GUI.
 */
int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));

    gtk_init(&argc, &argv);

    /* Load persisted settings. */
    Settings *settings = settings_load();
    if (!settings) return 1;

    /* Bring up the TTS engine with the saved voice parameters. */
    tts_init();
    tts_set_rate(settings->voice_rate);
    tts_set_pitch(settings->voice_pitch);

    /* Try to load the local model; the app works offline without it. */
    if (!llm_init()) {
        fprintf(stderr, "[neon_core] No GGUF model found in ./models/ - running in offline mode.\n");
    } else {
        fprintf(stderr, "[neon_core] Model loaded.\n");
    }

    ui_init(settings);

    gtk_main();

    llm_cleanup();
    tts_cleanup();
    free(settings);
    return 0;
}
