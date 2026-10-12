#ifndef TTS_H
#define TTS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <speak_lib.h>

void tts_init();
void tts_speak(const char *text);
void tts_set_rate(int rate);
void tts_set_pitch(int pitch);
void tts_cleanup();

#ifdef __cplusplus
}
#endif

#endif
