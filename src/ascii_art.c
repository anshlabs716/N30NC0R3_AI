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
    char *lines[ASCII_7_ROWS];
    for (int i = 0; i < ASCII_7_ROWS; i++) {
        lines[i] = calloc(ASCII_MAX_WIDTH * 2, sizeof(char));
        lines[i][0] = '\0';
    }
    
    size_t len = strlen(text);
    for (size_t j = 0; j < len; j++) {
        char c = text[j];
        int idx = char_to_index(c);
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
    
    // truncate
    if (width > 0) {
        for (int i = 0; i < ASCII_7_ROWS; i++) {
            if ((int)strlen(lines[i]) > width) {
                lines[i][width - 2] = '.';
                lines[i][width - 1] = '.';
                lines[i][width] = '\0';
            }
        }
    }
    
    char *result = calloc(ASCII_MAX_WIDTH * ASCII_7_ROWS + ASCII_7_ROWS, sizeof(char));
    result[0] = '\0';
    for (int i = 0; i < ASCII_7_ROWS; i++) {
        strcat(result, lines[i]);
        if (i < ASCII_7_ROWS - 1) strcat(result, "\n");
        free(lines[i]);
    }
    return result;
}

char* generate_ascii_art(const char *text, const char *style, int width, int height, int color) {
    char *base = text_to_ascii(text, width);
    char *result = calloc(ASCII_MAX_WIDTH * ASCII_MAX_HEIGHT * 2, sizeof(char));
    result[0] = '\0';
    
    // simple style application - block style by default
    char *token = strtok(base, "\n");
    int line_count = 0;
    while (token != NULL && line_count < height) {
        char *styled = calloc(strlen(token) * 2, sizeof(char));
        styled[0] = '\0';
        size_t tlen = strlen(token);
        for (size_t i = 0; i < tlen; i++) {
            if (token[i] != ' ') {
                if (color) {
                    char marker[8];
                    int col = (i + line_count) % 7;
                    snprintf(marker, sizeof(marker), "§%d%c", col, token[i]);
                    strcat(styled, marker);
                } else {
                    strcat(styled, "█");
                }
            } else {
                strcat(styled, " ");
            }
        }
        strcat(result, styled);
        if (line_count < height - 1 && token != NULL) strcat(result, "\n");
        free(styled);
        token = strtok(NULL, "\n");
        line_count++;
    }
    
    free(base);
    return result;
}

char* generate_pattern(const char *pattern_type, int width, int height, int color) {
    char *result = calloc(ASCII_MAX_WIDTH * ASCII_MAX_HEIGHT * 2, sizeof(char));
    result[0] = '\0';
    
    srand((unsigned)time(NULL));
    const char *chars = " .:-=+*#%@";
    int char_count = strlen(chars);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (strcmp(pattern_type, "diamond") == 0) {
                double dx = x - width / 2.0;
                double dy = y - height / 2.0;
                double dist = fabs(dx) + fabs(dy);
                int idx = (int)(dist / (width / 2.0) * char_count) % char_count;
                if (color) {
                    char marker[8];
                    snprintf(marker, sizeof(marker), "§%d%c", (x + y) % 7, chars[idx]);
                    strcat(result, marker);
                } else {
                    strcat(result, &chars[idx]);
                }
            } else {
                if (color) {
                    char marker[8];
                    int idx = (x + y) % char_count;
                    snprintf(marker, sizeof(marker), "§%d%c", (x + y) % 7, chars[idx]);
                    strcat(result, marker);
                } else {
                    char c[2] = {chars[(x + y) % char_count], '\0'};
                    strcat(result, c);
                }
            }
        }
        if (y < height - 1) strcat(result, "\n");
    }
    return result;
}
