#define _GNU_SOURCE
#include "terminal.h"
#include "themes.h"
#include "shortcuts.h"
#include <unistd.h>
#include <limits.h>

#define URL_REGEX_PATTERN "(https?|ftp)://[a-zA-Z0-9./_~:?#\\[\\]@!$&'()*+,;=%-]+"
#ifndef PCRE2_MULTILINE
#define PCRE2_MULTILINE 0x00000400u
#endif

static char *get_foreground_process_name(ZephysTab *tab) {
    if (!tab || !tab->terminal || tab->child_pid <= 0) return NULL;

    VtePty *pty = vte_terminal_get_pty(VTE_TERMINAL(tab->terminal));
    if (!pty) return NULL;

    int pty_fd = vte_pty_get_fd(pty);
    if (pty_fd < 0) return NULL;

    pid_t fg_pgrp = tcgetpgrp(pty_fd);
    if (fg_pgrp <= 0) return NULL;

    /* If foreground process group is the shell itself, no external command is running */
    if (fg_pgrp == tab->child_pid) {
        return NULL;
    }

    /* Read process command name from /proc/<fg_pgrp>/comm */
    char comm_path[64];
    snprintf(comm_path, sizeof(comm_path), "/proc/%d/comm", (int)fg_pgrp);
    char *comm = NULL;
    gsize length = 0;
    if (g_file_get_contents(comm_path, &comm, &length, NULL)) {
        g_strstrip(comm);
        if (*comm) {
            return comm;
        }
        g_free(comm);
    }

    return NULL;
}

void zephys_tab_update_title(ZephysTab *tab) {
    if (!tab || !tab->title_label) return;

    char *cwd = zephys_tab_get_cwd(tab);
    const char *home = g_get_home_dir();
    char *display_title = NULL;

    char *fg_process = get_foreground_process_name(tab);
    if (fg_process) {
        display_title = fg_process;
    } else if (cwd && home && g_strcmp0(cwd, home) == 0) {
        display_title = g_strdup("~");
    } else if (cwd && g_strcmp0(cwd, "/") == 0) {
        display_title = g_strdup("/");
    } else if (cwd && *cwd) {
        char *base = g_path_get_basename(cwd);
        display_title = g_strdup(base);
        g_free(base);
    } else {
        display_title = g_strdup("~");
    }

    gtk_label_set_text(GTK_LABEL(tab->title_label), display_title);
    if (cwd) {
        gtk_widget_set_tooltip_text(tab->title_label, cwd);
    }

    /* Update window title if this is the active tab */
    int current_page = gtk_notebook_get_current_page(tab->notebook);
    int tab_page = gtk_notebook_page_num(tab->notebook, tab->box);
    if (current_page == tab_page && tab->window) {
        char *win_title = g_strdup_printf("%s - Zephys Terminal", display_title);
        gtk_window_set_title(tab->window, win_title);
        g_free(win_title);
    }

    g_free(display_title);
    g_free(cwd);
}

static void on_child_exited(VteTerminal *terminal, int status, gpointer user_data) {
    (void)terminal;
    (void)status;
    ZephysTab *tab = (ZephysTab *)user_data;
    zephys_tab_close(tab);
}

static void on_window_title_changed(VteTerminal *terminal, gpointer user_data) {
    (void)terminal;
    ZephysTab *tab = (ZephysTab *)user_data;
    zephys_tab_update_title(tab);
}

static void on_uri_changed(VteTerminal *terminal, gpointer user_data) {
    ZephysTab *tab = (ZephysTab *)user_data;
    const char *uri = vte_terminal_get_current_file_uri(terminal);
    if (uri) {
        char *path = g_filename_from_uri(uri, NULL, NULL);
        if (path) {
            g_free(tab->cwd);
            tab->cwd = path;
        }
    }
    zephys_tab_update_title(tab);
}

static void on_spawn_completed(VteTerminal *terminal, GPid pid, GError *error, gpointer user_data) {
    (void)terminal;
    ZephysTab *tab = (ZephysTab *)user_data;
    if (error) {
        g_printerr("Failed to spawn shell: %s\n", error->message);
        g_error_free(error);
        return;
    }
    tab->child_pid = pid;
    zephys_tab_update_title(tab);
}

static void on_close_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    ZephysTab *tab = (ZephysTab *)user_data;
    zephys_tab_close(tab);
}

char *zephys_tab_get_cwd(ZephysTab *tab) {
    if (!tab) return g_strdup(g_get_home_dir());

    /* OSC 7 URI */
    const char *uri = vte_terminal_get_current_file_uri(VTE_TERMINAL(tab->terminal));
    if (uri) {
        char *path = g_filename_from_uri(uri, NULL, NULL);
        if (path) return path;
    }

    /* /proc/<pid>/cwd */
    if (tab->child_pid > 0) {
        char proc_link[64];
        snprintf(proc_link, sizeof(proc_link), "/proc/%d/cwd", (int)tab->child_pid);
        char target[PATH_MAX];
        ssize_t len = readlink(proc_link, target, sizeof(target) - 1);
        if (len != -1) {
            target[len] = '\0';
            return g_strdup(target);
        }
    }

    /* Cached directory */
    if (tab->cwd && *tab->cwd) {
        return g_strdup(tab->cwd);
    }

    /* Fallback */
    return g_strdup(g_get_home_dir());
}

void zephys_tab_open_in_file_manager(ZephysTab *tab) {
    char *cwd = zephys_tab_get_cwd(tab);
    if (!cwd) return;

    const char *fm = (tab->config && tab->config->file_manager) ? tab->config->file_manager : "xdg-open";
    char *argv[] = {(char *)fm, cwd, NULL};

    GError *err = NULL;
    if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err)) {
        g_printerr("Failed to open file manager: %s\n", err ? err->message : "unknown error");
        if (err) g_error_free(err);
    }

    g_free(cwd);
}

void zephys_tab_copy_path(ZephysTab *tab) {
    char *cwd = zephys_tab_get_cwd(tab);
    if (!cwd) return;

    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, cwd, -1);
    g_free(cwd);
}

static void update_tab_label_font(ZephysTab *tab) {
    if (!tab || !tab->title_label || !tab->config || !tab->config->font_name) return;

    PangoFontDescription *pfd = pango_font_description_from_string(tab->config->font_name);
    if (!pfd) return;

    int base_size = pango_font_description_get_size(pfd);
    if (pango_font_description_get_size_is_absolute(pfd)) {
        pango_font_description_set_absolute_size(pfd, base_size * tab->font_scale);
    } else {
        pango_font_description_set_size(pfd, (int)(base_size * tab->font_scale));
    }

    PangoAttrList *attrs = pango_attr_list_new();
    PangoAttribute *attr = pango_attr_font_desc_new(pfd);
    pango_attr_list_insert(attrs, attr);
    gtk_label_set_attributes(GTK_LABEL(tab->title_label), attrs);
    pango_attr_list_unref(attrs);
    pango_font_description_free(pfd);
}

void zephys_tab_zoom(ZephysTab *tab, double factor) {
    if (!tab || !tab->terminal) return;
    tab->font_scale *= factor;
    if (tab->font_scale < 0.2) tab->font_scale = 0.2;
    if (tab->font_scale > 5.0) tab->font_scale = 5.0;
    vte_terminal_set_font_scale(VTE_TERMINAL(tab->terminal), tab->font_scale);
    update_tab_label_font(tab);
}

void zephys_tab_zoom_reset(ZephysTab *tab) {
    if (!tab || !tab->terminal) return;
    tab->font_scale = 1.0;
    vte_terminal_set_font_scale(VTE_TERMINAL(tab->terminal), 1.0);
    update_tab_label_font(tab);
}

int zephys_notebook_get_tab_count(GtkNotebook *notebook) {
    if (!notebook) return 0;
    int total = gtk_notebook_get_n_pages(notebook);
    int count = 0;
    for (int i = 0; i < total; i++) {
        GtkWidget *p = gtk_notebook_get_nth_page(notebook, i);
        if (p && g_object_get_data(G_OBJECT(p), "zephys-tab")) {
            count++;
        }
    }
    return count;
}

static gboolean on_tab_label_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    ZephysTab *tab = (ZephysTab *)user_data;
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_MIDDLE) {
        zephys_tab_close(tab);
        return GDK_EVENT_STOP;
    }
    return GDK_EVENT_PROPAGATE;
}

void zephys_tab_close(ZephysTab *tab) {
    if (!tab) return;

    if (tab->title_timer_id > 0) {
        g_source_remove(tab->title_timer_id);
        tab->title_timer_id = 0;
    }

    GtkNotebook *notebook = tab->notebook;
    GtkWindow *window = tab->window;

    int page_num = gtk_notebook_page_num(notebook, tab->box);
    int real_count = zephys_notebook_get_tab_count(notebook);
    int cur = gtk_notebook_get_current_page(notebook);

    if (page_num >= 0) {
        /* If closing the currently active tab, switch to adjacent real tab first */
        if (real_count > 1 && cur == page_num) {
            if (page_num == real_count - 1) {
                gtk_notebook_set_current_page(notebook, page_num - 1);
            } else {
                gtk_notebook_set_current_page(notebook, page_num + 1);
            }
        }
        gtk_notebook_remove_page(notebook, page_num);
    }

    /* If no more tabs, close the window */
    if (zephys_notebook_get_tab_count(notebook) == 0) {
        if (window) {
            gtk_window_close(window);
        }
    }
}

static void on_tab_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ZephysTab *tab = (ZephysTab *)user_data;
    if (tab->title_timer_id > 0) {
        g_source_remove(tab->title_timer_id);
        tab->title_timer_id = 0;
    }
    g_free(tab->cwd);
    g_free(tab);
}

static gboolean on_tab_box_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)widget;
    ZephysTab *tab = (ZephysTab *)user_data;
    if (!tab || !tab->config) return FALSE;

    GdkRGBA fg, bg, cursor, palette[16];
    zephys_get_theme_palette(tab->config->theme_name, &fg, &bg, &cursor, palette);

    double alpha = tab->config->opacity;
    if (alpha < 0.1) alpha = 0.1;
    if (alpha > 1.0) alpha = 1.0;

    cairo_set_source_rgba(cr, bg.red, bg.green, bg.blue, alpha);
    cairo_paint(cr);

    return FALSE;
}

void zephys_tab_apply_config(ZephysTab *tab, ZephysConfig *config) {
    if (!tab || !tab->terminal || !config) return;

    tab->config = config;

    /* Theme colors */
    GdkRGBA fg, bg, cursor, palette[16];
    zephys_get_theme_palette(config->theme_name, &fg, &bg, &cursor, palette);

    if (config->opacity < 1.0) {
        bg.alpha = config->opacity;
    }

    vte_terminal_set_colors(VTE_TERMINAL(tab->terminal), &fg, &bg, palette, 16);
    vte_terminal_set_color_cursor(VTE_TERMINAL(tab->terminal), &cursor);

    /* Font */
    if (config->font_name) {
        PangoFontDescription *pfd = pango_font_description_from_string(config->font_name);
        vte_terminal_set_font(VTE_TERMINAL(tab->terminal), pfd);
        pango_font_description_free(pfd);
    }

    /* Cursor shape */
    VteCursorShape shape = VTE_CURSOR_SHAPE_BLOCK;
    if (config->cursor_shape) {
        if (g_ascii_strcasecmp(config->cursor_shape, "ibeam") == 0) {
            shape = VTE_CURSOR_SHAPE_IBEAM;
        } else if (g_ascii_strcasecmp(config->cursor_shape, "underline") == 0) {
            shape = VTE_CURSOR_SHAPE_UNDERLINE;
        }
    }
    vte_terminal_set_cursor_shape(VTE_TERMINAL(tab->terminal), shape);

    /* Cursor blink */
    vte_terminal_set_cursor_blink_mode(
        VTE_TERMINAL(tab->terminal),
        config->cursor_blink ? VTE_CURSOR_BLINK_ON : VTE_CURSOR_BLINK_OFF
    );

    /* Scrollback & Bell */
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(tab->terminal), config->scrollback_lines);
    vte_terminal_set_audible_bell(VTE_TERMINAL(tab->terminal), config->audible_bell);

    /* Padding (applied as inner margins on the terminal widget) */
    if (tab->terminal) {
        int pad = (config->padding >= 0 && config->padding <= 64) ? config->padding : 0;
        gtk_widget_set_margin_start(tab->terminal, pad);
        gtk_widget_set_margin_end(tab->terminal, pad);
        gtk_widget_set_margin_top(tab->terminal, pad);
        gtk_widget_set_margin_bottom(tab->terminal, pad);
    }
    if (tab->box) {
        gtk_container_set_border_width(GTK_CONTAINER(tab->box), 0);
        gtk_widget_queue_draw(tab->box);
    }

    /* Match tab label font to terminal */
    update_tab_label_font(tab);
}

ZephysTab *zephys_tab_new(GtkNotebook *notebook, GtkWindow *window, ZephysConfig *config, const char *initial_cwd) {
    ZephysTab *tab = g_new0(ZephysTab, 1);
    tab->notebook = notebook;
    tab->window = window;
    tab->config = config;
    tab->font_scale = 1.0;
    tab->cwd = initial_cwd ? g_strdup(initial_cwd) : NULL;

    /* Container */
    tab->box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    g_object_set_data(G_OBJECT(tab->box), "zephys-tab", tab);
    g_signal_connect(tab->box, "draw", G_CALLBACK(on_tab_box_draw), tab);

    /* Terminal widget */
    tab->terminal = vte_terminal_new();
    gtk_box_pack_start(GTK_BOX(tab->box), tab->terminal, TRUE, TRUE, 0);

    /* Apply configuration */
    zephys_tab_apply_config(tab, config);

    vte_terminal_set_mouse_autohide(VTE_TERMINAL(tab->terminal), TRUE);
    vte_terminal_set_allow_hyperlink(VTE_TERMINAL(tab->terminal), TRUE);

    /* URL pattern for clickable links */
    GError *regex_err = NULL;
    VteRegex *regex = vte_regex_new_for_match(URL_REGEX_PATTERN, -1, PCRE2_MULTILINE, &regex_err);
    if (regex) {
        int tag = vte_terminal_match_add_regex(VTE_TERMINAL(tab->terminal), regex, 0);
        vte_terminal_match_set_cursor_name(VTE_TERMINAL(tab->terminal), tag, "pointer");
        vte_regex_unref(regex);
    } else if (regex_err) {
        g_error_free(regex_err);
    }

    /* Tab header (label and close button, with middle-click support) */
    tab->tab_label_event_box = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(tab->tab_label_event_box), FALSE);
    gtk_widget_add_events(tab->tab_label_event_box, GDK_BUTTON_PRESS_MASK);

    tab->tab_label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(tab->tab_label_box), "zephys-tab-label-box");
    tab->title_label = gtk_label_new("~");
    gtk_label_set_ellipsize(GTK_LABEL(tab->title_label), PANGO_ELLIPSIZE_END);
    gtk_label_set_width_chars(GTK_LABEL(tab->title_label), 8);
    gtk_label_set_max_width_chars(GTK_LABEL(tab->title_label), 24);

    tab->close_button = gtk_button_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(tab->close_button), GTK_RELIEF_NONE);
    gtk_widget_set_focus_on_click(tab->close_button, FALSE);
    g_signal_connect(tab->close_button, "clicked", G_CALLBACK(on_close_button_clicked), tab);
    g_signal_connect(tab->close_button, "button-press-event", G_CALLBACK(on_tab_label_button_press), tab);

    gtk_box_pack_start(GTK_BOX(tab->tab_label_box), tab->title_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tab->tab_label_box), tab->close_button, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(tab->tab_label_event_box), tab->tab_label_box);
    g_signal_connect(tab->tab_label_event_box, "button-press-event", G_CALLBACK(on_tab_label_button_press), tab);
    gtk_widget_show_all(tab->tab_label_event_box);
    update_tab_label_font(tab);
    zephys_tab_update_title(tab);

    /* Spawn shell */
    const char *shell = g_getenv("SHELL");
    if (!shell || !*shell) {
        shell = "/bin/bash";
    }
    char *argv[] = {(char *)shell, NULL};

    const char *spawn_dir = tab->cwd ? tab->cwd : g_get_home_dir();

    vte_terminal_spawn_async(
        VTE_TERMINAL(tab->terminal),
        VTE_PTY_DEFAULT,
        spawn_dir,
        argv,
        NULL,
        G_SPAWN_SEARCH_PATH,
        NULL, NULL, NULL,
        -1,
        NULL,
        on_spawn_completed,
        tab
    );

    /* Signals */
    g_signal_connect(tab->terminal, "child-exited", G_CALLBACK(on_child_exited), tab);
    g_signal_connect(tab->terminal, "window-title-changed", G_CALLBACK(on_window_title_changed), tab);
    g_signal_connect(tab->terminal, "current-directory-uri-changed", G_CALLBACK(on_uri_changed), tab);
    g_signal_connect(tab->terminal, "key-press-event", G_CALLBACK(zephys_on_key_press), tab);
    g_signal_connect(tab->terminal, "button-press-event", G_CALLBACK(zephys_on_button_press), tab);
    g_signal_connect(tab->box, "destroy", G_CALLBACK(on_tab_destroy), tab);

    /* Add to notebook (insert before dummy '+' tab if present) */
    int dummy_index = -1;
    int n_pages = gtk_notebook_get_n_pages(notebook);
    for (int i = 0; i < n_pages; i++) {
        GtkWidget *p = gtk_notebook_get_nth_page(notebook, i);
        if (p && g_object_get_data(G_OBJECT(p), "zephys-dummy-page")) {
            dummy_index = i;
            break;
        }
    }

    int index;
    if (dummy_index >= 0) {
        index = gtk_notebook_insert_page(notebook, tab->box, tab->tab_label_event_box, dummy_index);
    } else {
        index = gtk_notebook_append_page(notebook, tab->box, tab->tab_label_event_box);
    }
    gtk_notebook_set_tab_reorderable(notebook, tab->box, TRUE);
    gtk_widget_show_all(tab->box);
    gtk_notebook_set_current_page(notebook, index);
    gtk_widget_grab_focus(tab->terminal);

    return tab;
}
