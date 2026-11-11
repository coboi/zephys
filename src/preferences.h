#ifndef ZEPHYS_PREFERENCES_H
#define ZEPHYS_PREFERENCES_H

#include <gtk/gtk.h>
#include "config.h"

void zephys_preferences_dialog_show(GtkWindow *parent, GtkNotebook *notebook, ZephysConfig *config);

#endif /* ZEPHYS_PREFERENCES_H */
