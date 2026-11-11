#ifndef ZEPHYS_TERMINAL_H
#define ZEPHYS_TERMINAL_H

#include <gtk/gtk.h>
#include <vte/vte.h>
#include "config.h"

typedef struct {
    GtkWidget *box;
    GtkWidget *terminal;
    GtkWidget *tab_label_event_box;
    GtkWidget *tab_label_box;
    GtkWidget *title_label;
    GtkWidget *close_button;
    GtkNotebook *notebook;
    GtkWindow *window;
    GPid child_pid;
    char *cwd;
    ZephysConfig *config;
    double font_scale;
    guint title_timer_id;
    gboolean is_closing;
} ZephysTab;

int zephys_notebook_get_tab_count(GtkNotebook *notebook);
ZephysTab *zephys_tab_new(GtkNotebook *notebook, GtkWindow *window, ZephysConfig *config, const char *initial_cwd);
void zephys_tab_close(ZephysTab *tab);
void zephys_tab_apply_config(ZephysTab *tab, ZephysConfig *config);
void zephys_tab_update_title(ZephysTab *tab);
char *zephys_tab_get_cwd(ZephysTab *tab);
void zephys_tab_open_in_file_manager(ZephysTab *tab);
void zephys_tab_copy_path(ZephysTab *tab);
void zephys_tab_zoom(ZephysTab *tab, double factor);
void zephys_tab_zoom_reset(ZephysTab *tab);

#endif /* ZEPHYS_TERMINAL_H */
