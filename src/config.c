#include "config.h"
#include <glib/gstdio.h>
#include <stdlib.h>

static char *get_config_path(void) {
    const char *config_dir = g_get_user_config_dir();
    return g_build_filename(config_dir, "zephys", "config.ini", NULL);
}

void zephys_config_ensure_default_file(void) {
    char *path = get_config_path();
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_free(path);
        return;
    }

    char *dir = g_path_get_dirname(path);
    g_mkdir_with_parents(dir, 0755);
    g_free(dir);

    const char *default_content =
        "# Zephys Terminal Configuration\n"
        "[appearance]\n"
        "font = Ubuntu Mono 12\n"
        "theme = dark\n"
        "opacity = 1.0\n"
        "cursor_shape = block\n"
        "cursor_blink = false\n"
        "padding = 0\n"
        "scrollback_lines = 10000\n"
        "audible_bell = false\n\n"
        "[integration]\n"
        "file_manager = xdg-open\n";

    g_file_set_contents(path, default_content, -1, NULL);
    g_free(path);
}

ZephysConfig *zephys_config_load(void) {
    ZephysConfig *cfg = g_new0(ZephysConfig, 1);
    cfg->font_name = g_strdup("Ubuntu Mono 12");
    cfg->theme_name = g_strdup("dark");
    cfg->opacity = 1.0;
    cfg->cursor_shape = g_strdup("block");
    cfg->cursor_blink = FALSE;
    cfg->padding = 0;
    cfg->scrollback_lines = 10000;
    cfg->audible_bell = FALSE;
    cfg->file_manager = g_strdup("xdg-open");

    char *path = get_config_path();
    GKeyFile *kf = g_key_file_new();

    if (g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        GError *err = NULL;

        char *font = g_key_file_get_string(kf, "appearance", "font", &err);
        if (font) {
            if (!err && *font) {
                g_free(cfg->font_name);
                cfg->font_name = font;
            } else {
                g_free(font);
            }
        }
        if (err) g_clear_error(&err);

        char *theme = g_key_file_get_string(kf, "appearance", "theme", &err);
        if (theme) {
            if (!err && *theme) {
                g_free(cfg->theme_name);
                cfg->theme_name = theme;
            } else {
                g_free(theme);
            }
        }
        if (err) g_clear_error(&err);

        double op = g_key_file_get_double(kf, "appearance", "opacity", &err);
        if (!err) {
            if (op < 0.2) op = 0.2;
            if (op > 1.0) op = 1.0;
            cfg->opacity = op;
        } else {
            g_clear_error(&err);
        }

        char *shape = g_key_file_get_string(kf, "appearance", "cursor_shape", &err);
        if (shape) {
            if (!err && *shape) {
                g_free(cfg->cursor_shape);
                cfg->cursor_shape = shape;
            } else {
                g_free(shape);
            }
        }
        if (err) g_clear_error(&err);

        gboolean blink = g_key_file_get_boolean(kf, "appearance", "cursor_blink", &err);
        if (!err) {
            cfg->cursor_blink = blink;
        } else {
            g_clear_error(&err);
        }

        int pad = g_key_file_get_integer(kf, "appearance", "padding", &err);
        if (!err) {
            cfg->padding = (pad >= 0 && pad <= 64) ? pad : 0;
        } else {
            g_clear_error(&err);
        }

        int sc = g_key_file_get_integer(kf, "appearance", "scrollback_lines", &err);
        if (!err) {
            cfg->scrollback_lines = (sc >= 0) ? sc : 10000;
        } else {
            g_clear_error(&err);
        }

        gboolean bell = g_key_file_get_boolean(kf, "appearance", "audible_bell", &err);
        if (!err) {
            cfg->audible_bell = bell;
        } else {
            g_clear_error(&err);
        }

        char *fm = g_key_file_get_string(kf, "integration", "file_manager", &err);
        if (fm) {
            if (!err && *fm) {
                g_free(cfg->file_manager);
                cfg->file_manager = fm;
            } else {
                g_free(fm);
            }
        }
        if (err) g_clear_error(&err);
    }

    g_key_file_free(kf);
    g_free(path);
    return cfg;
}

void zephys_config_save(ZephysConfig *config) {
    if (!config) return;

    char *path = get_config_path();
    GKeyFile *kf = g_key_file_new();

    /* Preserve existing keys */
    g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL);

    g_key_file_set_string(kf, "appearance", "font", config->font_name ? config->font_name : "Ubuntu Mono 12");
    g_key_file_set_string(kf, "appearance", "theme", config->theme_name ? config->theme_name : "dark");
    g_key_file_set_double(kf, "appearance", "opacity", config->opacity);
    g_key_file_set_string(kf, "appearance", "cursor_shape", config->cursor_shape ? config->cursor_shape : "block");
    g_key_file_set_boolean(kf, "appearance", "cursor_blink", config->cursor_blink);
    g_key_file_set_integer(kf, "appearance", "padding", config->padding);
    g_key_file_set_integer(kf, "appearance", "scrollback_lines", config->scrollback_lines);
    g_key_file_set_boolean(kf, "appearance", "audible_bell", config->audible_bell);

    if (config->file_manager) {
        g_key_file_set_string(kf, "integration", "file_manager", config->file_manager);
    }

    g_key_file_save_to_file(kf, path, NULL);
    g_key_file_free(kf);
    g_free(path);
}

void zephys_config_free(ZephysConfig *config) {
    if (!config) return;
    g_free(config->font_name);
    g_free(config->theme_name);
    g_free(config->cursor_shape);
    g_free(config->file_manager);
    g_free(config);
}
