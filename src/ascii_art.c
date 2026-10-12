#include "ascii_art.h"

const char *ASCII_COLOR_NAMES[7] = {"Red", "Orange", "Yellow", "Green", "Blue", "Indigo", "Violet"};
const int ASCII_COLORS_RGB[7][3] = {
    {255, 0, 0},      // Red
    {255, 127, 0},    // Orange
    {255, 255, 0},    // Yellow
    {0, 255, 0},      // Green
    {0, 0, 255},      // Blue
    {75, 0, 130},     // Indigo
    {139, 0, 255}     // Violet
};

typedef struct {
    const char *key;
    const char *pattern[ASCII_7_ROWS];
} FontChar;

static const FontChar FONT[] = {
    {"A", {"  ##  ", " #  # ", "#    #", "######", "#    #", "#    #", "#    #"}},
    {"B", {"##### ", "#    #", "#    #", "##### ", "#    #", "#    #", "##### "}},
    {"C", {" #### ", "#    #", "#     ", "#     ", "#     ", "#    #", " #### "}},
    {"D", {"##### ", "#    #", "#    #", "#    #", "#    #", "#    #", "##### "}},
    {"E", {"######", "#     ", "#     ", "##### ", "#     ", "#     ", "######"}},
    {"F", {"######", "#     ", "#     ", "##### ", "#     ", "#     ", "#     "}},
    {"G", {" #### ", "#    #", "#     ", "#  ###", "#    #", "#    #", " #### "}},
    {"H", {"#    #", "#    #", "#    #", "######", "#    #", "#    #", "#    #"}},
    {"I", {"######", "  ##  ", "  ##  ", "  ##  ", "  ##  ", "  ##  ", "######"}},
    {"J", {"######", "   #  ", "   #  ", "   #  ", "   #  ", "#  #  ", " ##   "}},
    {"K", {"#    #", "#   # ", "#  #  ", "###   ", "#  #  ", "#   # ", "#    #"}},
    {"L", {"#     ", "#     ", "#     ", "#     ", "#     ", "#     ", "######"}},
    {"M", {"#    #", "##  ##", "# ## #", "#    #", "#    #", "#    #", "#    #"}},
    {"N", {"#    #", "##   #", "# #  #", "#  # #", "#   ##", "#    #", "#    #"}},
    {"O", {" #### ", "#    #", "#    #", "#    #", "#    #", "#    #", " #### "}},
    {"P", {"##### ", "#    #", "#    #", "##### ", "#     ", "#     ", "#     "}},
    {"Q", {" #### ", "#    #", "#    #", "#    #", "# #  #", "#  # #", " #### "}},
    {"R", {"##### ", "#    #", "#    #", "##### ", "#  #  ", "#   # ", "#    #"}},
    {"S", {" #####", "#     ", "#     ", " #### ", "     #", "     #", "##### "}},
    {"T", {"######", "  ##  ", "  ##  ", "  ##  ", "  ##  ", "  ##  ", "  ##  "}},
    {"U", {"#    #", "#    #", "#    #", "#    #", "#    #", "#    #", " #### "}},
    {"V", {"#    #", "#    #", "#    #", "#    #", "#    #", " #  # ", "  ##  "}},
    {"W", {"#    #", "#    #", "#    #", "# #  #", "# ## #", "##  ##", "#    #"}},
    {"X", {"#    #", " #  # ", "  ##  ", "  ##  ", "  ##  ", " #  # ", "#    #"}},
    {"Y", {"#    #", " #  # ", "  ##  ", "  ##  ", "  ##  ", "  ##  ", "  ##  "}},
    {"Z", {"######", "    # ", "   #  ", "  #   ", " #    ", "#     ", "######"}},
    {"0", {" #### ", "#    #", "#  # #", "# #  #", "#  # #", "#    #", " #### "}},
    {"1", {"  ##  ", " ###  ", "  ##  ", "  ##  ", "  ##  ", "  ##  ", "##### "}},
    {"2", {" #### ", "#    #", "    # ", "   #  ", "  #   ", " #    ", "######"}},
    {"3", {" #### ", "#    #", "    # ", " ###  ", "    # ", "#    #", " #### "}},
    {"4", {"   #  ", "  ##  ", " # #  ", "#  #  ", "######", "   #  ", "   #  "}},
    {"5", {"######", "#     ", "#     ", "##### ", "     #", "     #", "##### "}},
    {"6", {" #### ", "#    #", "#     ", "##### ", "#    #", "#    #", " #### "}},
    {"7", {"######", "     #", "    # ", "   #  ", "  #   ", " #    ", "#     "}},
    {"8", {" #### ", "#    #", "#    #", " #### ", "#    #", "#    #", " #### "}},
    {"9", {" #### ", "#    #", "#    #", " #####", "     #", "    # ", " #### "}},
    {" ", {"      ", "      ", "      ", "      ", "      ", "      ", "      "}},
    {".", {"      ", "      ", "      ", "      ", "      ", "  ##  ", "  ##  "}},
    {"!", {"  #   ", "  #   ", "  #   ", "  #   ", "  #   ", "      ", "  #   "}},
    {"?", {" #### ", "#    #", "    # ", "   #  ", "  #   ", "      ", "  #   "}},
    {"#", {" #  # ", " #  # ", "######", " #  # ", "######", " #  # ", " #  # "}},
    {"*", {"      ", " #  # ", "  ##  ", " #  # ", "      ", "      ", "      "}},
    {"+", {"      ", "  #   ", "  #   ", "######", "  #   ", "  #   ", "      "}},
    {"-", {"      ", "      ", "      ", "######", "      ", "      ", "      "}},
    {"_", {"      ", "      ", "      ", "      ", "      ", "      ", "######"}},
    {"=", {"      ", "######", "      ", "######", "      ", "      ", "      "}},
    {"/", {"     #", "    # ", "   #  ", "  #   ", " #    ", "#     ", "      "}},
    {"\\", {"#     ", " #    ", "  #   ", "   #  ", "    # ", "     #", "      "}},
    {"|", {"  #   ", "  #   ", "  #   ", "  #   ", "  #   ", "  #   ", "  #   "}},
    {":", {"      ", "  ##  ", "  ##  ", "      ", "  ##  ", "  ##  ", "      "}},
    {";", {"      ", "  ##  ", "  ##  ", "      ", "  ##  ", "  ##  ", " #    "}},
    {",", {"      ", "      ", "      ", "      ", "      ", "  ##  ", " #    "}},
    {"(", {"   #  ", "  #   ", " #    ", " #    ", " #    ", "  #   ", "   #  "}},
    {")", {" #    ", "  #   ", "   #  ", "   #  ", "   #  ", "  #   ", " #    "}},
    {"[", {"#####", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"}},
    {"]", {"#####", "    #", "    #", "    #", "    #", "    #", "#####"}},
    {"{", {"  ## ", " #   ", " #   ", "##   ", " #   ", " #   ", "  ## "}},
    {"}", {" ##  ", "   # ", "   # ", "  ## ", "   # ", "   # ", " ##  "}},
    {"@", {" ### ", "#   #", "#  ##", "# # #", "#  ##", "#   #", " ### "}},
    {NULL, {NULL}}
};

static int char_to_index(const char c) {
    char upper = (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
    for (int i = 0; FONT[i].key != NULL; i++) {
        if (FONT[i].key[0] == upper) return i;
    }
    return -1;
}

char* text_to_ascii(const char *text, int width) {
    if (!text) text = "";
    size_t len = strlen(text);
    if (len > ASCII_MAX_WIDTH) len = ASCII_MAX_WIDTH;   /* keep the art sane */

    /* Every glyph is at most 6 columns wide; also leave room for truncation. */
    size_t per_line = len * 6 + 1;
    if (width > 0 && (size_t)width + 1 > per_line) per_line = (size_t)width + 1;

    char *lines[ASCII_7_ROWS];
    for (int i = 0; i < ASCII_7_ROWS; i++) {
        lines[i] = calloc(per_line, sizeof(char));
        if (!lines[i]) {
            for (int k = 0; k < i; k++) free(lines[k]);
            return NULL;
        }
        lines[i][0] = '\0';
    }

    for (size_t j = 0; j < len; j++) {
        int idx = char_to_index(text[j]);
        if (idx >= 0) {
            for (int i = 0; i < ASCII_7_ROWS; i++) {
                strcat(lines[i], FONT[idx].pattern[i]);
            }
        } else {
            // unknown character
            for (int i = 0; i < ASCII_7_ROWS; i++) {
                strcat(lines[i], "? ");
            }
        }
    }

    // truncate (only when there is room for two dots)
    if (width >= 2) {
        for (int i = 0; i < ASCII_7_ROWS; i++) {
            if ((int)strlen(lines[i]) > width) {
                lines[i][width - 2] = '.';
                lines[i][width - 1] = '.';
                lines[i][width] = '\0';
            }
        }
    }

    size_t cap = per_line * ASCII_7_ROWS + ASCII_7_ROWS + 1;
    char *result = calloc(cap, sizeof(char));
    if (!result) {
        for (int i = 0; i < ASCII_7_ROWS; i++) free(lines[i]);
        return NULL;
    }
    result[0] = '\0';
    for (int i = 0; i < ASCII_7_ROWS; i++) {
        strcat(result, lines[i]);
        if (i < ASCII_7_ROWS - 1) strcat(result, "\n");
        free(lines[i]);
    }
    return result;
}

char* generate_ascii_art(const char *text, const char *style, int width, int height, int color) {
    (void)style;  /* style is reserved for future use */
    char *base = text_to_ascii(text, width);
    if (!base) return NULL;

    /* A colour marker ("§0X") costs up to 4 bytes per character. */
    size_t cap = strlen(base) * 4 + 64;
    char *result = calloc(cap, sizeof(char));
    if (!result) { free(base); return NULL; }
    result[0] = '\0';

    if (height < 1) height = 1;

    // simple style application - block style by default
    char *token = strtok(base, "\n");
    int line_count = 0;
    while (token != NULL && line_count < height) {
        size_t tlen = strlen(token);
        char *styled = calloc(tlen * 5 + 2, sizeof(char));
        if (!styled) break;
        styled[0] = '\0';
        for (size_t i = 0; i < tlen; i++) {
            if (token[i] != ' ') {
                if (color) {
                    char marker[8];
                    int col = (int)((i + (size_t)line_count) % 7);
                    snprintf(marker, sizeof(marker), "\xC2\xA7%d%c", col, token[i]);
                    strcat(styled, marker);
                } else {
                    strcat(styled, "\xE2\x96\x88"); /* U+2588 full block */
                }
            } else {
                strcat(styled, " ");
            }
        }
        strcat(result, styled);
        if (line_count < height - 1) strcat(result, "\n");
        free(styled);
        token = strtok(NULL, "\n");
        line_count++;
    }

    free(base);
    return result;
}

char* generate_pattern(const char *pattern_type, int width, int height, int color) {
    if (width  < 1) width  = 1;
    if (height < 1) height = 1;
    if (width  > ASCII_MAX_WIDTH)  width  = ASCII_MAX_WIDTH;
    if (height > ASCII_MAX_HEIGHT) height = ASCII_MAX_HEIGHT;

    size_t cap = (size_t)width * (size_t)height * 5 + (size_t)height + 16;
    char *result = calloc(cap, sizeof(char));
    if (!result) return NULL;
    result[0] = '\0';

    const char *chars = " .:-=+*#%@";
    int char_count = (int)strlen(chars);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx;
            if (strcmp(pattern_type, "diamond") == 0) {
                double dx = x - width / 2.0;
                double dy = y - height / 2.0;
                double dist = fabs(dx) + fabs(dy);
                idx = (int)(dist / (width / 2.0) * char_count) % char_count;
                if (idx < 0) idx += char_count;
            } else {
                idx = (x + y) % char_count;
            }

            if (color) {
                char marker[8];
                snprintf(marker, sizeof(marker), "\xC2\xA7%d%c", (x + y) % 7, chars[idx]);
                strcat(result, marker);
            } else {
                char c[2] = { chars[idx], '\0' };
                strcat(result, c);
            }
        }
        if (y < height - 1) strcat(result, "\n");
    }
    return result;
}
