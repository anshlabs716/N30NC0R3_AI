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
 * neoncore v3.0
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

    /* Try a model; cloud if enabled & reachable, else the local one. */
    llm_set_cloud_enabled(settings->use_cloud);
    {
        char ep[512], mdl[128], lbl[32];
        settings_cloud_resolve(settings, ep, sizeof(ep), mdl, sizeof(mdl), lbl, sizeof(lbl));
        llm_set_cloud_config(ep, mdl, settings->cloud_api_key, lbl);
    }
    if (!llm_init()) {
        fprintf(stderr, "[neoncore] No model available - running in offline mode.\n");
    } else {
        fprintf(stderr, "[neoncore] Model backend: %s\n", llm_backend_name());
    }

    ui_init(settings);

    gtk_main();

    llm_cleanup();
    tts_cleanup();
    free(settings);
    return 0;
}
