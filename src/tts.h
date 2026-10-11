#ifndef TTS_H
#define TTS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>

/* espeak-ng installs its header under either /usr/include or
 * /usr/include/espeak-ng depending on the distribution. */
#if defined(__has_include)
#  if __has_include(<espeak-ng/speak_lib.h>)
#    include <espeak-ng/speak_lib.h>
#  elif __has_include(<speak_lib.h>)
#    include <speak_lib.h>
#  endif
#else
#  include <speak_lib.h>
#endif

void tts_init();
void tts_speak(const char *text);
void tts_set_rate(int rate);
void tts_set_pitch(int pitch);
void tts_cleanup();

#ifdef __cplusplus
}
#endif

#endif
