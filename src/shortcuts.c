#include "shortcuts.h"
#include "preferences.h"
#include <gdk/gdkkeysyms.h>

static void on_menu_copy(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    ZephysTab *tab = (ZephysTab *)user_data;
    vte_terminal_copy_clipboard_format(VTE_TERMINAL(tab->terminal), VTE_FORMAT_TEXT);
}

static void on_menu_paste(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    ZephysTab *tab = (ZephysTab *)user_data;
    vte_terminal_paste_clipboard(VTE_TERMINAL(tab->terminal));
}

static void on_menu_open_fm(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    ZephysTab *tab = (ZephysTab *)user_data;
    zephys_tab_open_in_file_manager(tab);
}

static void on_menu_copy_path(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    ZephysTab *tab = (ZephysTab *)user_data;
    zephys_tab_copy_path(tab);
}

static void on_menu_new_tab(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    ZephysTab *tab = (ZephysTab *)user_data;
    char *cwd = zephys_tab_get_cwd(tab);
    zephys_tab_new(tab->notebook, tab->window, tab->config, cwd);
    g_free(cwd);
}

static void on_menu_close_tab(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    ZephysTab *tab = (ZephysTab *)user_data;
    zephys_tab_close(tab);
}

static void on_menu_preferences(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    ZephysTab *tab = (ZephysTab *)user_data;
    zephys_preferences_dialog_show(tab->window, tab->notebook, tab->config);
}

void zephys_show_context_menu(ZephysTab *tab, GdkEventButton *event) {
    GtkWidget *menu = gtk_menu_new();
    g_signal_connect(menu, "selection-done", G_CALLBACK(gtk_widget_destroy), NULL);

    /* Copy */
    GtkWidget *item_copy = gtk_menu_item_new_with_mnemonic("_Copy");
    g_signal_connect(item_copy, "activate", G_CALLBACK(on_menu_copy), tab);
    gtk_widget_set_sensitive(item_copy, vte_terminal_get_has_selection(VTE_TERMINAL(tab->terminal)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_copy);

    /* Paste */
    GtkWidget *item_paste = gtk_menu_item_new_with_mnemonic("_Paste");
    g_signal_connect(item_paste, "activate", G_CALLBACK(on_menu_paste), tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_paste);

    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    /* Open in File Manager */
    GtkWidget *item_open_fm = gtk_menu_item_new_with_mnemonic("Open in _File Manager");
    g_signal_connect(item_open_fm, "activate", G_CALLBACK(on_menu_open_fm), tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_open_fm);

    /* Copy directory path */
    GtkWidget *item_copy_path = gtk_menu_item_new_with_mnemonic("Copy _Directory Path");
    g_signal_connect(item_copy_path, "activate", G_CALLBACK(on_menu_copy_path), tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_copy_path);

    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    /* New Tab */
    GtkWidget *item_new_tab = gtk_menu_item_new_with_mnemonic("_New Tab");
    g_signal_connect(item_new_tab, "activate", G_CALLBACK(on_menu_new_tab), tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_new_tab);

    /* Close Tab */
    GtkWidget *item_close_tab = gtk_menu_item_new_with_mnemonic("_Close Tab");
    g_signal_connect(item_close_tab, "activate", G_CALLBACK(on_menu_close_tab), tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_close_tab);

    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    /* Preferences */
    GtkWidget *item_prefs = gtk_menu_item_new_with_mnemonic("Pre_ferences...");
    g_signal_connect(item_prefs, "activate", G_CALLBACK(on_menu_preferences), tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_prefs);

    gtk_widget_show_all(menu);

    if (event) {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
    } else {
        gtk_menu_popup_at_widget(GTK_MENU(menu), tab->terminal, GDK_GRAVITY_CENTER, GDK_GRAVITY_CENTER, NULL);
    }
}

static gboolean on_title_update_idle(gpointer user_data) {
    ZephysTab *tab = (ZephysTab *)user_data;
    tab->title_timer_id = 0;
    zephys_tab_update_title(tab);
    return G_SOURCE_REMOVE;
}

gboolean zephys_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    ZephysTab *tab = (ZephysTab *)user_data;
    guint state = event->state & gtk_accelerator_get_default_mod_mask();
    guint keyval = event->keyval;

    /* Fullscreen toggle (F11) */
    if (keyval == GDK_KEY_F11 && !(state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
        if (tab->window) {
            GdkWindow *gdk_win = gtk_widget_get_window(GTK_WIDGET(tab->window));
            if (gdk_win) {
                GdkWindowState win_state = gdk_window_get_state(gdk_win);
                if (win_state & GDK_WINDOW_STATE_FULLSCREEN) {
                    gtk_window_unfullscreen(tab->window);
                } else {
                    gtk_window_fullscreen(tab->window);
                }
            } else {
                gtk_window_fullscreen(tab->window);
            }
            return TRUE;
        }
    }

    /* Multiline newline (Shift + Enter) */
    if ((keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter || keyval == GDK_KEY_ISO_Enter) &&
        (state & GDK_SHIFT_MASK) && !(state & GDK_CONTROL_MASK)) {
        vte_terminal_feed_child(VTE_TERMINAL(tab->terminal), "\n", 1);
        return TRUE;
    }

    /* Schedule title update on command submit or interrupt */
    if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter ||
        ((state & GDK_CONTROL_MASK) && (keyval == GDK_KEY_c || keyval == GDK_KEY_C ||
                                       keyval == GDK_KEY_d || keyval == GDK_KEY_D))) {
        if (tab->title_timer_id > 0) {
            g_source_remove(tab->title_timer_id);
        }
        tab->title_timer_id = g_timeout_add(100, on_title_update_idle, tab);
    }

    /* Erase word backward (Shift/Ctrl + Backspace sends ^W) */
    if (keyval == GDK_KEY_BackSpace) {
        if ((state & GDK_SHIFT_MASK) || (state & GDK_CONTROL_MASK)) {
            vte_terminal_feed_child(VTE_TERMINAL(tab->terminal), "\x17", 1);
            return TRUE;
        }
    }

    /* Erase word forward (Shift/Ctrl + Delete sends Alt+D) */
    if (keyval == GDK_KEY_Delete || keyval == GDK_KEY_KP_Delete) {
        if ((state & GDK_SHIFT_MASK) || (state & GDK_CONTROL_MASK)) {
            vte_terminal_feed_child(VTE_TERMINAL(tab->terminal), "\033d", 2);
            return TRUE;
        }
    }

    /* Ctrl + Shift shortcuts */
    if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
        switch (keyval) {
            case GDK_KEY_C:
            case GDK_KEY_c:
                vte_terminal_copy_clipboard_format(VTE_TERMINAL(tab->terminal), VTE_FORMAT_TEXT);
                return TRUE;

            case GDK_KEY_V:
            case GDK_KEY_v:
                vte_terminal_paste_clipboard(VTE_TERMINAL(tab->terminal));
                return TRUE;

            case GDK_KEY_T:
            case GDK_KEY_t: {
                char *cwd = zephys_tab_get_cwd(tab);
                zephys_tab_new(tab->notebook, tab->window, tab->config, cwd);
                g_free(cwd);
                return TRUE;
            }

            case GDK_KEY_W:
            case GDK_KEY_w:
                zephys_tab_close(tab);
                return TRUE;

            case GDK_KEY_O:
            case GDK_KEY_o:
                zephys_tab_open_in_file_manager(tab);
                return TRUE;

            case GDK_KEY_P:
            case GDK_KEY_p:
                zephys_preferences_dialog_show(tab->window, tab->notebook, tab->config);
                return TRUE;

            default:
                break;
        }
    }

    /* Navigation and font scaling */
    if (state == GDK_CONTROL_MASK) {
        switch (keyval) {
            case GDK_KEY_Page_Up: {
                int cur = gtk_notebook_get_current_page(tab->notebook);
                int total = zephys_notebook_get_tab_count(tab->notebook);
                if (total > 1) {
                    gtk_notebook_set_current_page(tab->notebook, (cur - 1 + total) % total);
                }
                return TRUE;
            }

            case GDK_KEY_Page_Down: {
                int cur = gtk_notebook_get_current_page(tab->notebook);
                int total = zephys_notebook_get_tab_count(tab->notebook);
                if (total > 1) {
                    gtk_notebook_set_current_page(tab->notebook, (cur + 1) % total);
                }
                return TRUE;
            }

            case GDK_KEY_plus:
            case GDK_KEY_equal:
            case GDK_KEY_KP_Add:
                zephys_tab_zoom(tab, 1.1);
                return TRUE;

            case GDK_KEY_minus:
            case GDK_KEY_KP_Subtract:
                zephys_tab_zoom(tab, 1.0 / 1.1);
                return TRUE;

            case GDK_KEY_0:
            case GDK_KEY_KP_0:
                zephys_tab_zoom_reset(tab);
                return TRUE;

            case GDK_KEY_Left:
            case GDK_KEY_KP_Left:
                /* Move backward one word (Alt+B) */
                vte_terminal_feed_child(VTE_TERMINAL(tab->terminal), "\033b", 2);
                return TRUE;

            case GDK_KEY_Right:
            case GDK_KEY_KP_Right:
                /* Move forward one word (Alt+F) */
                vte_terminal_feed_child(VTE_TERMINAL(tab->terminal), "\033f", 2);
                return TRUE;

            case GDK_KEY_comma:
                zephys_preferences_dialog_show(tab->window, tab->notebook, tab->config);
                return TRUE;

            default:
                break;
        }
    }

    /* Alt + 1..9: Switch to tab */
    if (state == GDK_MOD1_MASK) {
        if (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_9) {
            int target_idx = keyval - GDK_KEY_1;
            if (target_idx < zephys_notebook_get_tab_count(tab->notebook)) {
                gtk_notebook_set_current_page(tab->notebook, target_idx);
                return TRUE;
            }
        }
    }

    return FALSE;
}

static gboolean is_safe_url_scheme(const char *url) {
    if (!url) return FALSE;
    return (g_ascii_strncasecmp(url, "http://", 7) == 0 ||
            g_ascii_strncasecmp(url, "https://", 8) == 0 ||
            g_ascii_strncasecmp(url, "ftp://", 6) == 0);
}

gboolean zephys_on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    ZephysTab *tab = (ZephysTab *)user_data;

    /* Context menu */
    if (event->button == GDK_BUTTON_SECONDARY) {
        zephys_show_context_menu(tab, event);
        return TRUE;
    }

    /* Ctrl + click to open URL */
    if (event->button == GDK_BUTTON_PRIMARY && (event->state & GDK_CONTROL_MASK)) {
        char *url = vte_terminal_match_check_event(VTE_TERMINAL(tab->terminal), (GdkEvent *)event, NULL);
        if (!url) {
            url = vte_terminal_hyperlink_check_event(VTE_TERMINAL(tab->terminal), (GdkEvent *)event);
        }

        if (url) {
            if (is_safe_url_scheme(url)) {
                GError *err = NULL;
                gtk_show_uri_on_window(tab->window, url, event->time, &err);
                if (err) {
                    g_printerr("Failed to open URL: %s\n", err->message);
                    g_error_free(err);
                }
            } else {
                g_printerr("Blocked URL with untrusted scheme: %s\n", url);
            }
            g_free(url);
            return TRUE;
        }
    }

    return FALSE;
}
