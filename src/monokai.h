#ifndef ZEPHYS_MONOKAI_H
#define ZEPHYS_MONOKAI_H

#include <gdk/gdk.h>

/* Monokai Color Palette definitions */
static inline void zephys_get_monokai_palette(GdkRGBA *fg, GdkRGBA *bg, GdkRGBA *cursor, GdkRGBA palette[16]) {
    /* Foreground: #F8F8F2 */
    gdk_rgba_parse(fg, "#F8F8F2");

    /* Background: #272822 */
    gdk_rgba_parse(bg, "#272822");

    /* Cursor: #F8F8F0 */
    gdk_rgba_parse(cursor, "#F8F8F0");

    /* 16 ANSI Palette */
    const char *hex_colors[16] = {
        /* Normal (0-7) */
        "#272822", /* 0: Black */
        "#F92672", /* 1: Red */
        "#A6E22E", /* 2: Green */
        "#F4BF75", /* 3: Yellow */
        "#66D9EF", /* 4: Blue */
        "#AE81FF", /* 5: Magenta */
        "#A1EFE4", /* 6: Cyan */
        "#F8F8F2", /* 7: White */
        /* Bright (8-15) */
        "#75715E", /* 8: Bright Black */
        "#F92672", /* 9: Bright Red */
        "#A6E22E", /* 10: Bright Green */
        "#E6DB74", /* 11: Bright Yellow */
        "#66D9EF", /* 12: Bright Blue */
        "#AE81FF", /* 13: Bright Magenta */
        "#A1EFE4", /* 14: Bright Cyan */
        "#F9F8F5"  /* 15: Bright White */
    };

    for (int i = 0; i < 16; i++) {
        gdk_rgba_parse(&palette[i], hex_colors[i]);
    }
}

#endif /* ZEPHYS_MONOKAI_H */
