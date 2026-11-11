#ifndef ZEPHYS_SHORTCUTS_H
#define ZEPHYS_SHORTCUTS_H

#include <gtk/gtk.h>
#include "terminal.h"

gboolean zephys_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
gboolean zephys_on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
void zephys_show_context_menu(ZephysTab *tab, GdkEventButton *event);

#endif /* ZEPHYS_SHORTCUTS_H */
