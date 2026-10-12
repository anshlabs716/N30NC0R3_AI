#ifndef ASCII_ART_H
#define ASCII_ART_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define ASCII_MAX_WIDTH 200
#define ASCII_MAX_HEIGHT 50
#define ASCII_7_ROWS 7

// 7-color markers (Red, Orange, Yellow, Green, Blue, Indigo, Violet)
extern const char *ASCII_COLOR_NAMES[7];
extern const int ASCII_COLORS_RGB[7][3];

char* text_to_ascii(const char *text, int width);
char* generate_ascii_art(const char *text, const char *style, int width, int height, int color);
char* generate_pattern(const char *pattern_type, int width, int height, int color);

#ifdef __cplusplus
}
#endif

#endif
