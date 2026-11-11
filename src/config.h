#ifndef ZEPHYS_CONFIG_H
#define ZEPHYS_CONFIG_H

#include <glib.h>

typedef struct {
    char *font_name;
    char *theme_name;    /* "dark" or "light" */
    double opacity;      /* 0.5 to 1.0 */
    char *cursor_shape;  /* "block", "ibeam", "underline" */
    gboolean cursor_blink;
    int padding;
    int scrollback_lines;
    gboolean audible_bell;
    char *file_manager;
} ZephysConfig;

ZephysConfig *zephys_config_load(void);
void zephys_config_save(ZephysConfig *config);
void zephys_config_free(ZephysConfig *config);
void zephys_config_ensure_default_file(void);

#endif /* ZEPHYS_CONFIG_H */
