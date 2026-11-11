#ifndef ZEPHYS_MAIN_H
#define ZEPHYS_MAIN_H

#include <gtk/gtk.h>
#include "config.h"

typedef struct {
    GtkNotebook *notebook;
    GtkWindow *window;
    ZephysConfig *config;
    GtkWidget *dummy_page;
} AppState;

GtkWidget *zephys_window_create(GtkApplication *app, ZephysConfig *config, const char *initial_cwd, gboolean add_initial_tab);
GtkWidget *zephys_window_new(ZephysConfig *config, const char *initial_cwd);

#endif /* ZEPHYS_MAIN_H */
