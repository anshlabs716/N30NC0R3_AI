#include "tts.h"

void tts_init() {
    espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, NULL, 0);
    espeak_SetVoiceByName("en");
}

void tts_speak(const char *text) {
    if (!text) return;
    espeak_Synth(text, strlen(text) + 1, 0, POS_CHARACTER, 0, 
                 espeakCHARS_UTF8 | espeakENDPAUSE, NULL, NULL);
}

void tts_set_rate(int rate) {
    espeak_SetParameter(espeakRATE, rate, 0);
}

void tts_set_pitch(int pitch) {
    espeak_SetParameter(espeakPITCH, pitch, 0);
}

void tts_cleanup() {
    espeak_Terminate();
}
