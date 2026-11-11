#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include "main.h"
#include "terminal.h"
#include "config.h"
#include "themes.h"

static GtkApplication *global_app = NULL;

static void on_new_tab_button_clicked(GtkButton *btn, gpointer user_data);
static void on_page_reordered(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data);

static void update_window_title_for_page(AppState *app, GtkWidget *page) {
    if (!app || !app->window || !page) return;

    if (g_object_get_data(G_OBJECT(page), "zephys-dummy-page")) {
        return;
    }

    ZephysTab *tab = (ZephysTab *)g_object_get_data(G_OBJECT(page), "zephys-tab");
    const char *title = NULL;
    if (tab && tab->title_label) {
        title = gtk_label_get_text(GTK_LABEL(tab->title_label));
    } else if (app->notebook) {
        GtkWidget *tab_label = gtk_notebook_get_tab_label(app->notebook, page);
        if (GTK_IS_BIN(tab_label)) {
            tab_label = gtk_bin_get_child(GTK_BIN(tab_label));
        }
        if (GTK_IS_BOX(tab_label)) {
            GList *children = gtk_container_get_children(GTK_CONTAINER(tab_label));
            if (children && GTK_IS_LABEL(children->data)) {
                title = gtk_label_get_text(GTK_LABEL(children->data));
            }
            if (children) g_list_free(children);
        }
    }

    if (title && *title) {
        char *win_title = g_strdup_printf("%s - Zephys Terminal", title);
        gtk_window_set_title(app->window, win_title);
        g_free(win_title);
    } else {
        gtk_window_set_title(app->window, "Zephys Terminal");
    }
}

static void on_page_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data) {
    (void)page_num;
    AppState *app = (AppState *)user_data;
    if (!page || !app) return;

    if (g_object_get_data(G_OBJECT(page), "zephys-dummy-page")) {
        int real_count = zephys_notebook_get_tab_count(notebook);
        if (real_count > 0) {
            int target_idx = real_count - 1;
            gtk_notebook_set_current_page(notebook, target_idx);
        }
        return;
    }

    update_window_title_for_page(app, page);
}

static void on_new_tab_button_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AppState *app = (AppState *)user_data;
    int current_page = gtk_notebook_get_current_page(app->notebook);
    char *cwd = NULL;
    if (current_page >= 0) {
        GtkWidget *page = gtk_notebook_get_nth_page(app->notebook, current_page);
        if (page) {
            ZephysTab *tab = (ZephysTab *)g_object_get_data(G_OBJECT(page), "zephys-tab");
            if (tab) {
                cwd = zephys_tab_get_cwd(tab);
            }
        }
    }
    zephys_tab_new(app->notebook, app->window, app->config, cwd);
    g_free(cwd);
}

static gboolean on_btn_add_tab_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    AppState *app = (AppState *)user_data;
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_MIDDLE) {
        on_new_tab_button_clicked(NULL, app);
        return GDK_EVENT_STOP;
    }
    return GDK_EVENT_PROPAGATE;
}

static void on_page_reordered(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data) {
    (void)child;
    (void)page_num;
    AppState *app = (AppState *)user_data;
    if (!app || !app->dummy_page) return;

    int dummy_idx = gtk_notebook_page_num(notebook, app->dummy_page);
    int total = gtk_notebook_get_n_pages(notebook);
    if (dummy_idx >= 0 && dummy_idx != total - 1) {
        g_signal_handlers_block_by_func(notebook, on_page_reordered, app);
        gtk_notebook_reorder_child(notebook, app->dummy_page, -1);
        g_signal_handlers_unblock_by_func(notebook, on_page_reordered, app);
    }
}

typedef struct {
    GtkWidget *notebook;
    GtkWidget *child;
} PageAddedData;

static gboolean idle_activate_added_page(gpointer user_data) {
    PageAddedData *data = (PageAddedData *)user_data;
    if (GTK_IS_NOTEBOOK(data->notebook) && GTK_IS_WIDGET(data->child)) {
        int idx = gtk_notebook_page_num(GTK_NOTEBOOK(data->notebook), data->child);
        if (idx >= 0) {
            gtk_notebook_set_current_page(GTK_NOTEBOOK(data->notebook), idx);
            ZephysTab *tab = (ZephysTab *)g_object_get_data(G_OBJECT(data->child), "zephys-tab");
            if (tab && tab->terminal) {
                gtk_widget_grab_focus(tab->terminal);
            }
        }
    }
    g_object_unref(data->notebook);
    g_object_unref(data->child);
    g_free(data);
    return G_SOURCE_REMOVE;
}

static void on_page_added(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data) {
    (void)page_num;
    AppState *app = (AppState *)user_data;
    if (!app) return;

    if (g_object_get_data(G_OBJECT(child), "zephys-dummy-page")) {
        return;
    }

    ZephysTab *tab = (ZephysTab *)g_object_get_data(G_OBJECT(child), "zephys-tab");
    if (tab) {
        tab->notebook = app->notebook;
        tab->window = app->window;
    }

    gtk_notebook_set_tab_reorderable(notebook, child, TRUE);
    gtk_notebook_set_tab_detachable(notebook, child, TRUE);

    if (app->dummy_page) {
        int dummy_idx = gtk_notebook_page_num(notebook, app->dummy_page);
        int total = gtk_notebook_get_n_pages(notebook);
        if (dummy_idx >= 0 && dummy_idx != total - 1) {
            g_signal_handlers_block_by_func(notebook, on_page_reordered, app);
            gtk_notebook_reorder_child(notebook, app->dummy_page, -1);
            g_signal_handlers_unblock_by_func(notebook, on_page_reordered, app);
        }
    }

    PageAddedData *data = g_new0(PageAddedData, 1);
    data->notebook = GTK_WIDGET(notebook);
    data->child = child;
    g_object_ref(data->notebook);
    g_object_ref(data->child);
    g_idle_add(idle_activate_added_page, data);

    update_window_title_for_page(app, child);
}

typedef struct {
    GtkWidget *window;
    int page_num;
} PageRemovedData;

static gboolean idle_handle_page_removed(gpointer user_data) {
    PageRemovedData *data = (PageRemovedData *)user_data;
    GtkWidget *window = data->window;

    if (GTK_IS_WINDOW(window) && !gtk_widget_in_destruction(window)) {
        AppState *app = (AppState *)g_object_get_data(G_OBJECT(window), "zephys-app");
        if (app && app->notebook) {
            int real_count = zephys_notebook_get_tab_count(app->notebook);
            if (real_count == 0) {
                gtk_window_close(GTK_WINDOW(window));
            } else {
                int cur = gtk_notebook_get_current_page(app->notebook);
                GtkWidget *cur_page = (cur >= 0) ? gtk_notebook_get_nth_page(app->notebook, cur) : NULL;

                if (!cur_page || g_object_get_data(G_OBJECT(cur_page), "zephys-dummy-page")) {
                    int target_idx = data->page_num;
                    if (target_idx >= real_count) {
                        target_idx = real_count - 1;
                    }
                    if (target_idx < 0) {
                        target_idx = 0;
                    }
                    gtk_notebook_set_current_page(app->notebook, target_idx);
                    cur_page = gtk_notebook_get_nth_page(app->notebook, target_idx);
                }

                if (cur_page) {
                    ZephysTab *active_tab = (ZephysTab *)g_object_get_data(G_OBJECT(cur_page), "zephys-tab");
                    if (active_tab && active_tab->terminal) {
                        gtk_widget_grab_focus(active_tab->terminal);
                    }
                    update_window_title_for_page(app, cur_page);
                }
            }
        }
    }

    g_object_unref(window);
    g_free(data);
    return G_SOURCE_REMOVE;
}

static void on_page_removed(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data) {
    (void)notebook;
    (void)child;
    AppState *app = (AppState *)user_data;
    if (!app || !app->window) return;

    PageRemovedData *data = g_new0(PageRemovedData, 1);
    data->window = GTK_WIDGET(app->window);
    data->page_num = (int)page_num;
    g_object_ref(data->window);
    g_idle_add(idle_handle_page_removed, data);
}

static GtkNotebook *on_create_window(GtkNotebook *source_notebook, GtkWidget *page, gint x, gint y, gpointer user_data) {
    (void)source_notebook;
    (void)page;
    AppState *source_app = (AppState *)user_data;
    if (!source_app || !global_app) return NULL;

    GtkWidget *new_window = zephys_window_create(global_app, source_app->config, NULL, FALSE);
    if (!new_window) return NULL;

    AppState *new_app = (AppState *)g_object_get_data(G_OBJECT(new_window), "zephys-app");
    if (!new_app || !new_app->notebook) {
        gtk_widget_destroy(new_window);
        return NULL;
    }

    gtk_window_move(GTK_WINDOW(new_window), x, y);
    gtk_widget_show_all(new_window);

    return new_app->notebook;
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    g_object_set_data(G_OBJECT(widget), "zephys-app", NULL);
}

static gboolean on_window_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)widget;
    (void)user_data;
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    return FALSE;
}

static char *resolve_working_directory(const char *input_path, const char *caller_cwd) {
    if (!input_path || !*input_path) {
        if (caller_cwd && *caller_cwd) {
            return g_strdup(caller_cwd);
        }
        return g_get_current_dir();
    }

    char *target_path = NULL;
    if (g_str_has_prefix(input_path, "file://")) {
        target_path = g_filename_from_uri(input_path, NULL, NULL);
    }
    if (!target_path) {
        target_path = g_strdup(input_path);
    }

    char *canon = NULL;
    if (g_path_is_absolute(target_path)) {
        canon = g_canonicalize_filename(target_path, NULL);
    } else {
        const char *base = (caller_cwd && *caller_cwd) ? caller_cwd : g_get_home_dir();
        canon = g_canonicalize_filename(target_path, base);
    }
    g_free(target_path);

    if (g_file_test(canon, G_FILE_TEST_IS_DIR)) {
        return canon;
    } else if (g_file_test(canon, G_FILE_TEST_EXISTS)) {
        char *dirname = g_path_get_dirname(canon);
        char *result = g_canonicalize_filename(dirname, NULL);
        g_free(dirname);
        g_free(canon);
        return result;
    } else {
        g_free(canon);
        if (caller_cwd && *caller_cwd) {
            return g_strdup(caller_cwd);
        }
        return g_get_current_dir();
    }
}

GtkWidget *zephys_window_create(GtkApplication *app, ZephysConfig *config, const char *initial_cwd, gboolean add_initial_tab) {
    /* Main window */
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Zephys Terminal");
    gtk_window_set_default_size(GTK_WINDOW(window), 840, 520);

    /* RGBA visual for transparency */
    GdkScreen *screen = gtk_widget_get_screen(window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual && gdk_screen_is_composited(screen)) {
        gtk_widget_set_visual(window, visual);
    }
    gtk_widget_set_app_paintable(window, TRUE);
    g_signal_connect(window, "draw", G_CALLBACK(on_window_draw), NULL);

    /* Terminal icon with fallback */
    if (gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "org.xfce.terminal")) {
        gtk_window_set_default_icon_name("org.xfce.terminal");
    } else {
        gtk_window_set_default_icon_name("utilities-terminal");
    }

    /* Notebook tabs */
    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
    gtk_notebook_set_group_name(GTK_NOTEBOOK(notebook), "zephys-tab-group");

    AppState *app_state = g_new0(AppState, 1);
    app_state->notebook = GTK_NOTEBOOK(notebook);
    app_state->window = GTK_WINDOW(window);
    app_state->config = config;

    g_object_set_data_full(G_OBJECT(window), "zephys-app", app_state, g_free);

    g_signal_connect(notebook, "page-reordered", G_CALLBACK(on_page_reordered), app_state);
    g_signal_connect(notebook, "page-added", G_CALLBACK(on_page_added), app_state);
    g_signal_connect(notebook, "page-removed", G_CALLBACK(on_page_removed), app_state);
    g_signal_connect(notebook, "create-window", G_CALLBACK(on_create_window), app_state);
    g_signal_connect(notebook, "switch-page", G_CALLBACK(on_page_switched), app_state);

    /* Dummy '+' tab placed at the end */
    GtkWidget *dummy_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(dummy_page);
    g_object_set_data(G_OBJECT(dummy_page), "zephys-dummy-page", GINT_TO_POINTER(1));
    app_state->dummy_page = dummy_page;

    GtkWidget *btn_add_tab = gtk_button_new_from_icon_name("list-add-symbolic", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(btn_add_tab), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(btn_add_tab, "New Tab (Ctrl+Shift+T)");
    gtk_widget_set_focus_on_click(btn_add_tab, FALSE);
    gtk_widget_set_name(btn_add_tab, "zephys-btn-new-tab");
    gtk_widget_set_valign(btn_add_tab, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(btn_add_tab, GTK_ALIGN_CENTER);
    g_signal_connect(btn_add_tab, "clicked", G_CALLBACK(on_new_tab_button_clicked), app_state);
    g_signal_connect(btn_add_tab, "button-press-event", G_CALLBACK(on_btn_add_tab_button_press), app_state);
    gtk_widget_show(btn_add_tab);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dummy_page, btn_add_tab);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), dummy_page, FALSE);
    gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(notebook), dummy_page, FALSE);

    gtk_container_add(GTK_CONTAINER(window), notebook);

    if (add_initial_tab) {
        zephys_tab_new(GTK_NOTEBOOK(notebook), GTK_WINDOW(window), config, initial_cwd);
    }

    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    return window;
}

GtkWidget *zephys_window_new(ZephysConfig *config, const char *initial_cwd) {
    if (!global_app) return NULL;
    GtkWidget *window = zephys_window_create(global_app, config, initial_cwd, TRUE);
    gtk_widget_show_all(window);
    return window;
}

static int on_command_line(GApplication *gapp, GApplicationCommandLine *cmdline, gpointer user_data) {
    int argc;
    char **argv = g_application_command_line_get_arguments(cmdline, &argc);
    ZephysConfig *config = (ZephysConfig *)user_data;

    for (int i = 1; i < argc; i++) {
        if (g_strcmp0(argv[i], "-h") == 0 || g_strcmp0(argv[i], "--help") == 0) {
            g_application_command_line_print(cmdline,
                "Usage: zephys [OPTIONS] [DIRECTORY]\n\n"
                "Clean terminal emulator built with GTK3 and VTE.\n\n"
                "Options:\n"
                "  -d, --working-directory=DIR  Set initial working directory\n"
                "  -v, --version                Show version information\n"
                "  -h, --help                   Show this help message\n");
            g_strfreev(argv);
            return 0;
        }
        if (g_strcmp0(argv[i], "-v") == 0 || g_strcmp0(argv[i], "--version") == 0) {
            g_application_command_line_print(cmdline, "Zephys Terminal 1.0\n");
            g_strfreev(argv);
            return 0;
        }
    }

    const char *cli_dir = NULL;
    for (int i = 1; i < argc; i++) {
        if (g_strcmp0(argv[i], "-d") == 0 && i + 1 < argc) {
            cli_dir = argv[++i];
        } else if (g_str_has_prefix(argv[i], "-d=")) {
            cli_dir = argv[i] + 3;
        } else if (g_strcmp0(argv[i], "--working-directory") == 0 && i + 1 < argc) {
            cli_dir = argv[++i];
        } else if (g_str_has_prefix(argv[i], "--working-directory=")) {
            cli_dir = argv[i] + 20;
        } else if (argv[i][0] != '-') {
            cli_dir = argv[i];
        }
    }

    const char *caller_cwd = g_application_command_line_get_cwd(cmdline);
    char *target_dir = resolve_working_directory(cli_dir, caller_cwd);

    GtkWidget *window = zephys_window_create(GTK_APPLICATION(gapp), config, target_dir, TRUE);
    g_free(target_dir);
    g_strfreev(argv);

    gtk_widget_show_all(window);
    return 0;
}

static void on_activate(GApplication *gapp, gpointer user_data) {
    ZephysConfig *config = (ZephysConfig *)user_data;
    GtkWidget *window = zephys_window_create(GTK_APPLICATION(gapp), config, NULL, TRUE);
    gtk_widget_show_all(window);
}

static void on_startup(GApplication *gapp, gpointer user_data) {
    (void)gapp;
    ZephysConfig *config = (ZephysConfig *)user_data;
    zephys_apply_tab_css(config->theme_name, config->opacity);
}

int main(int argc, char *argv[]) {
    /* Load configuration */
    zephys_config_ensure_default_file();
    ZephysConfig *config = zephys_config_load();

    GtkApplication *app = gtk_application_new("org.zephys.terminal", G_APPLICATION_HANDLES_COMMAND_LINE);
    global_app = app;

    g_signal_connect(app, "startup", G_CALLBACK(on_startup), config);
    g_signal_connect(app, "command-line", G_CALLBACK(on_command_line), config);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), config);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    global_app = NULL;
    zephys_config_free(config);
    return status;
}
