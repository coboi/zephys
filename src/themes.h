#ifndef ZEPHYS_THEMES_H
#define ZEPHYS_THEMES_H

#include <gdk/gdk.h>
#include <gtk/gtk.h>

void zephys_get_monokai_dark_palette(GdkRGBA *fg, GdkRGBA *bg, GdkRGBA *cursor, GdkRGBA palette[16]);
void zephys_get_monokai_light_palette(GdkRGBA *fg, GdkRGBA *bg, GdkRGBA *cursor, GdkRGBA palette[16]);
void zephys_get_theme_palette(const char *theme_name, GdkRGBA *fg, GdkRGBA *bg, GdkRGBA *cursor, GdkRGBA palette[16]);
void zephys_apply_tab_css(const char *theme_name, double opacity);

#endif /* ZEPHYS_THEMES_H */
