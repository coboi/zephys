#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include "terminal.h"
#include "config.h"
#include "themes.h"

typedef struct {
    GtkNotebook *notebook;
    GtkWindow *window;
    ZephysConfig *config;
    GtkWidget *dummy_page;
} AppState;

static void on_new_tab_button_clicked(GtkButton *btn, gpointer user_data);

static void on_page_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data) {
    (void)page_num;
    AppState *app = (AppState *)user_data;
    if (!page) return;

    if (g_object_get_data(G_OBJECT(page), "zephys-dummy-page")) {
        return;
    }

    ZephysTab *tab = (ZephysTab *)g_object_get_data(G_OBJECT(page), "zephys-tab");
    const char *title = NULL;
    if (tab && tab->title_label) {
        title = gtk_label_get_text(GTK_LABEL(tab->title_label));
    } else {
        GtkWidget *tab_label = gtk_notebook_get_tab_label(notebook, page);
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

static gboolean on_window_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)widget;
    (void)user_data;
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    return FALSE;
}

static void print_usage(const char *prog) {
    g_print("Usage: %s [OPTIONS] [DIRECTORY]\n\n", prog);
    g_print("Clean terminal emulator built with GTK3 and VTE.\n\n");
    g_print("Options:\n");
    g_print("  -d, --working-directory=DIR  Set initial working directory\n");
    g_print("  -v, --version                Show version information\n");
    g_print("  -h, --help                   Show this help message\n");
}

static char *resolve_working_directory(const char *input_path) {
    if (!input_path || !*input_path) {
        return g_get_current_dir();
    }

    char *target_path = NULL;
    if (g_str_has_prefix(input_path, "file://")) {
        target_path = g_filename_from_uri(input_path, NULL, NULL);
    }
    if (!target_path) {
        target_path = g_strdup(input_path);
    }

    char *result = NULL;
    if (g_file_test(target_path, G_FILE_TEST_IS_DIR)) {
        result = g_canonicalize_filename(target_path, NULL);
    } else if (g_file_test(target_path, G_FILE_TEST_EXISTS)) {
        char *dirname = g_path_get_dirname(target_path);
        result = g_canonicalize_filename(dirname, NULL);
        g_free(dirname);
    } else {
        char *canon = g_canonicalize_filename(target_path, NULL);
        if (g_file_test(canon, G_FILE_TEST_IS_DIR)) {
            result = canon;
        } else {
            g_free(canon);
            result = g_get_current_dir();
        }
    }

    g_free(target_path);
    return result;
}

int main(int argc, char *argv[]) {
    const char *cli_dir = NULL;

    /* Check for help and version before initializing GTK */
    for (int i = 1; i < argc; i++) {
        if (g_strcmp0(argv[i], "-h") == 0 || g_strcmp0(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (g_strcmp0(argv[i], "-v") == 0 || g_strcmp0(argv[i], "--version") == 0) {
            g_print("Zephys Terminal 1.0\n");
            return 0;
        }
    }

    gtk_init(&argc, &argv);

    /* Parse remaining CLI options */
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

    /* Load configuration */
    zephys_config_ensure_default_file();
    ZephysConfig *config = zephys_config_load();

    /* Tab styling for theme and opacity */
    zephys_apply_tab_css(config->theme_name, config->opacity);

    /* Main window */
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
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

    AppState *app = g_new0(AppState, 1);
    app->notebook = GTK_NOTEBOOK(notebook);
    app->window = GTK_WINDOW(window);
    app->config = config;

    g_signal_connect(notebook, "page-reordered", G_CALLBACK(on_page_reordered), app);

    /* Dummy '+' tab placed at the end */
    GtkWidget *dummy_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(dummy_page);
    g_object_set_data(G_OBJECT(dummy_page), "zephys-dummy-page", GINT_TO_POINTER(1));
    app->dummy_page = dummy_page;

    GtkWidget *btn_add_tab = gtk_button_new_from_icon_name("list-add-symbolic", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(btn_add_tab), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(btn_add_tab, "New Tab (Ctrl+Shift+T)");
    gtk_widget_set_focus_on_click(btn_add_tab, FALSE);
    gtk_widget_set_name(btn_add_tab, "zephys-btn-new-tab");
    gtk_widget_set_valign(btn_add_tab, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(btn_add_tab, GTK_ALIGN_CENTER);
    g_signal_connect(btn_add_tab, "clicked", G_CALLBACK(on_new_tab_button_clicked), app);
    g_signal_connect(btn_add_tab, "button-press-event", G_CALLBACK(on_btn_add_tab_button_press), app);
    gtk_widget_show(btn_add_tab);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dummy_page, btn_add_tab);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), dummy_page, FALSE);

    gtk_container_add(GTK_CONTAINER(window), notebook);

    /* Determine initial working directory */
    char *initial_cwd = resolve_working_directory(cli_dir);
    if (initial_cwd) {
        g_chdir(initial_cwd);
    }

    /* Initial tab */
    zephys_tab_new(GTK_NOTEBOOK(notebook), GTK_WINDOW(window), config, initial_cwd);
    g_free(initial_cwd);

    g_signal_connect(notebook, "switch-page", G_CALLBACK(on_page_switched), app);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    zephys_config_free(config);
    g_free(app);
    return 0;
}
