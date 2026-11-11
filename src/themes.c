#include "themes.h"

/* Monokai Dark Palette */
void zephys_get_monokai_dark_palette(GdkRGBA *fg, GdkRGBA *bg, GdkRGBA *cursor, GdkRGBA palette[16]) {
    gdk_rgba_parse(fg, "#F8F8F2");
    gdk_rgba_parse(bg, "#272822");
    gdk_rgba_parse(cursor, "#F8F8F0");

    const char *hex_colors[16] = {
        "#272822", "#F92672", "#A6E22E", "#F4BF75",
        "#66D9EF", "#AE81FF", "#A1EFE4", "#F8F8F2",
        "#75715E", "#F92672", "#A6E22E", "#E6DB74",
        "#66D9EF", "#AE81FF", "#A1EFE4", "#F9F8F5"
    };
    for (int i = 0; i < 16; i++) {
        gdk_rgba_parse(&palette[i], hex_colors[i]);
    }
}

/* Monokai Light Palette */
void zephys_get_monokai_light_palette(GdkRGBA *fg, GdkRGBA *bg, GdkRGBA *cursor, GdkRGBA palette[16]) {
    gdk_rgba_parse(fg, "#272822");
    gdk_rgba_parse(bg, "#FAF8F2");
    gdk_rgba_parse(cursor, "#272822");

    const char *hex_colors[16] = {
        "#FAF8F2", "#E0115F", "#6B9B08", "#C48A00",
        "#008CB8", "#7C3AED", "#009E94", "#272822",
        "#939084", "#F92672", "#82B414", "#D6A000",
        "#00A8C6", "#8A45FF", "#00B4A4", "#1E1F1C"
    };
    for (int i = 0; i < 16; i++) {
        gdk_rgba_parse(&palette[i], hex_colors[i]);
    }
}

void zephys_get_theme_palette(const char *theme_name, GdkRGBA *fg, GdkRGBA *bg, GdkRGBA *cursor, GdkRGBA palette[16]) {
    if (theme_name && g_ascii_strcasecmp(theme_name, "light") == 0) {
        zephys_get_monokai_light_palette(fg, bg, cursor, palette);
    } else {
        zephys_get_monokai_dark_palette(fg, bg, cursor, palette);
    }
}

static GtkCssProvider *global_css_provider = NULL;

void zephys_apply_tab_css(const char *theme_name, double opacity) {
    gboolean is_light = (theme_name && g_ascii_strcasecmp(theme_name, "light") == 0);

    if (opacity < 0.1) opacity = 0.1;
    if (opacity > 1.0) opacity = 1.0;

    char op[16];
    g_ascii_formatd(op, sizeof(op), "%.2f", opacity);

    char *css = NULL;
    if (is_light) {
        css = g_strdup_printf(
            "window, notebook, notebook.frame, notebook > stack { "
            "   background-color: transparent; "
            "   border: none; "
            "   box-shadow: none; "
            "   padding: 0; "
            "   margin: 0; "
            "} "
            "notebook > header { "
            "   background-color: rgba(236, 231, 217, %s); "
            "   border-bottom: 1px solid rgba(216, 210, 194, %s); "
            "   box-shadow: none; "
            "   padding: 0; "
            "   margin: 0; "
            "} "
            "notebook > header.top > tabs > tab { "
            "   background-color: rgba(236, 231, 217, %s); "
            "   color: #79766D; "
            "   padding: 5px 14px; "
            "   border: 1px solid transparent; "
            "   border-bottom: none; "
            "   border-top-left-radius: 4px; "
            "   border-top-right-radius: 4px; "
            "   box-shadow: none; "
            "} "
            "notebook > header.top > tabs > tab:hover { "
            "   background-color: rgba(244, 239, 226, %s); "
            "   color: #272822; "
            "} "
            "notebook > header.top > tabs > tab:checked { "
            "   background-color: rgba(250, 248, 242, %s); "
            "   color: #272822; "
            "   border: 1px solid rgba(216, 210, 194, %s); "
            "   border-bottom: 1px solid rgba(250, 248, 242, %s); "
            "   margin-bottom: -1px; "
            "   box-shadow: none; "
            "} "
            "notebook > header > button, notebook > header.top > tabs > tab button { "
            "   padding: 2px 6px; "
            "   min-height: 16px; "
            "   min-width: 16px; "
            "   border-radius: 3px; "
            "   border: none; "
            "   box-shadow: none; "
            "   background: transparent; "
            "   color: #79766D; "
            "} "
            "notebook > header > button:hover, notebook > header.top > tabs > tab button:hover { "
            "   background-color: rgba(216, 210, 194, %s); "
            "   color: #272822; "
            "} ",
            op, op, op, op, op, op, op, op
        );
    } else {
        css = g_strdup_printf(
            "window, notebook, notebook.frame, notebook > stack { "
            "   background-color: transparent; "
            "   border: none; "
            "   box-shadow: none; "
            "   padding: 0; "
            "   margin: 0; "
            "} "
            "notebook > header { "
            "   background-color: rgba(30, 31, 28, %s); "
            "   border-bottom: 1px solid rgba(62, 61, 50, %s); "
            "   box-shadow: none; "
            "   padding: 0; "
            "   margin: 0; "
            "} "
            "notebook > header.top > tabs > tab { "
            "   background-color: rgba(30, 31, 28, %s); "
            "   color: #75715e; "
            "   padding: 5px 14px; "
            "   border: 1px solid transparent; "
            "   border-bottom: none; "
            "   border-top-left-radius: 4px; "
            "   border-top-right-radius: 4px; "
            "   box-shadow: none; "
            "} "
            "notebook > header.top > tabs > tab:hover { "
            "   background-color: rgba(45, 46, 39, %s); "
            "   color: #f8f8f2; "
            "} "
            "notebook > header.top > tabs > tab:checked { "
            "   background-color: rgba(39, 40, 34, %s); "
            "   color: #f8f8f2; "
            "   border: 1px solid rgba(62, 61, 50, %s); "
            "   border-bottom: 1px solid rgba(39, 40, 34, %s); "
            "   margin-bottom: -1px; "
            "   box-shadow: none; "
            "} "
            "notebook > header > button, notebook > header.top > tabs > tab button { "
            "   padding: 2px 6px; "
            "   min-height: 16px; "
            "   min-width: 16px; "
            "   border-radius: 3px; "
            "   border: none; "
            "   box-shadow: none; "
            "   background: transparent; "
            "   color: #75715e; "
            "} "
            "notebook > header > button:hover, notebook > header.top > tabs > tab button:hover { "
            "   background-color: rgba(62, 61, 50, %s); "
            "   color: #f8f8f2; "
            "} ",
            op, op, op, op, op, op, op, op
        );
    }

    if (!global_css_provider) {
        global_css_provider = gtk_css_provider_new();
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(global_css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    gtk_css_provider_load_from_data(global_css_provider, css, -1, NULL);
    g_free(css);
}
