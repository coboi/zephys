#include "preferences.h"
#include "terminal.h"
#include "themes.h"

typedef struct {
    GtkWindow *parent;
    GtkNotebook *notebook;
    ZephysConfig *config;
    GtkWidget *btn_font;
    GtkWidget *combo_theme;
    GtkWidget *scale_opacity;
    GtkWidget *combo_cursor;
    GtkWidget *chk_blink;
} PrefsContext;

static void apply_preferences(PrefsContext *ctx) {
    /* 1. Font */
    char *font = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(ctx->btn_font));
    if (font) {
        g_free(ctx->config->font_name);
        ctx->config->font_name = font;
    }

    /* 2. Theme */
    const char *theme = gtk_combo_box_get_active_id(GTK_COMBO_BOX(ctx->combo_theme));
    if (theme) {
        g_free(ctx->config->theme_name);
        ctx->config->theme_name = g_strdup(theme);
    }

    /* 3. Opacity */
    ctx->config->opacity = gtk_range_get_value(GTK_RANGE(ctx->scale_opacity));

    /* 4. Cursor Shape */
    const char *cursor = gtk_combo_box_get_active_id(GTK_COMBO_BOX(ctx->combo_cursor));
    if (cursor) {
        g_free(ctx->config->cursor_shape);
        ctx->config->cursor_shape = g_strdup(cursor);
    }

    /* 5. Cursor Blink */
    ctx->config->cursor_blink = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctx->chk_blink));

    /* Update tab style */
    zephys_apply_tab_css(ctx->config->theme_name, ctx->config->opacity);

    /* Update open tabs */
    int n_pages = gtk_notebook_get_n_pages(ctx->notebook);
    for (int i = 0; i < n_pages; i++) {
        GtkWidget *page = gtk_notebook_get_nth_page(ctx->notebook, i);
        if (page) {
            ZephysTab *tab = (ZephysTab *)g_object_get_data(G_OBJECT(page), "zephys-tab");
            if (tab) {
                zephys_tab_apply_config(tab, ctx->config);
            }
        }
    }

    /* Save configuration */
    zephys_config_save(ctx->config);

    if (ctx->parent) {
        gtk_widget_queue_draw(GTK_WIDGET(ctx->parent));
    }
}

static gchar *format_opacity_value(GtkScale *scale, gdouble value, gpointer user_data) {
    (void)scale;
    (void)user_data;
    return g_strdup_printf("%d%%", (int)(value * 100));
}

static void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    PrefsContext *ctx = (PrefsContext *)user_data;

    if (response_id == GTK_RESPONSE_APPLY) {
        apply_preferences(ctx);
    } else if (response_id == GTK_RESPONSE_OK) {
        apply_preferences(ctx);
        gtk_widget_destroy(GTK_WIDGET(dialog));
    } else {
        gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}

static GtkWidget *active_preferences_dialog = NULL;

static void on_dialog_destroy(GtkWidget *dialog, gpointer user_data) {
    (void)dialog;
    active_preferences_dialog = NULL;
    PrefsContext *ctx = (PrefsContext *)user_data;
    g_free(ctx);
}

void zephys_preferences_dialog_show(GtkWindow *parent, GtkNotebook *notebook, ZephysConfig *config) {
    if (active_preferences_dialog) {
        gtk_window_present(GTK_WINDOW(active_preferences_dialog));
        return;
    }

    PrefsContext *ctx = g_new0(PrefsContext, 1);
    ctx->parent = parent;
    ctx->notebook = notebook;
    ctx->config = config;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Preferences",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Apply", GTK_RESPONSE_APPLY,
        "_OK", GTK_RESPONSE_OK,
        NULL
    );
    active_preferences_dialog = dialog;

    gtk_window_set_default_size(GTK_WINDOW(dialog), 380, 260);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 8);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    int row = 0;

    /* 1. Font */
    GtkWidget *lbl_font = gtk_label_new("Font:");
    gtk_widget_set_halign(lbl_font, GTK_ALIGN_START);
    ctx->btn_font = gtk_font_button_new_with_font(config->font_name ? config->font_name : "Ubuntu Mono 12");
    gtk_font_chooser_set_show_preview_entry(GTK_FONT_CHOOSER(ctx->btn_font), TRUE);

    gtk_grid_attach(GTK_GRID(grid), lbl_font, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ctx->btn_font, 1, row, 1, 1);
    row++;

    /* 2. Theme */
    GtkWidget *lbl_theme = gtk_label_new("Theme:");
    gtk_widget_set_halign(lbl_theme, GTK_ALIGN_START);
    ctx->combo_theme = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ctx->combo_theme), "dark", "Monokai Dark");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ctx->combo_theme), "light", "Monokai Light");

    if (config->theme_name && g_ascii_strcasecmp(config->theme_name, "light") == 0) {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(ctx->combo_theme), "light");
    } else {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(ctx->combo_theme), "dark");
    }

    gtk_grid_attach(GTK_GRID(grid), lbl_theme, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ctx->combo_theme, 1, row, 1, 1);
    row++;

    /* 3. Opacity */
    GtkWidget *lbl_opacity = gtk_label_new("Opacity:");
    gtk_widget_set_halign(lbl_opacity, GTK_ALIGN_START);
    ctx->scale_opacity = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.20, 1.0, 0.05);
    gtk_range_set_value(GTK_RANGE(ctx->scale_opacity), config->opacity);
    gtk_scale_set_draw_value(GTK_SCALE(ctx->scale_opacity), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(ctx->scale_opacity), GTK_POS_RIGHT);
    g_signal_connect(ctx->scale_opacity, "format-value", G_CALLBACK(format_opacity_value), NULL);

    gtk_grid_attach(GTK_GRID(grid), lbl_opacity, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ctx->scale_opacity, 1, row, 1, 1);
    row++;

    /* 4. Cursor Shape */
    GtkWidget *lbl_cursor = gtk_label_new("Cursor shape:");
    gtk_widget_set_halign(lbl_cursor, GTK_ALIGN_START);
    ctx->combo_cursor = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ctx->combo_cursor), "block", "Block (█)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ctx->combo_cursor), "ibeam", "I-Beam (|)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ctx->combo_cursor), "underline", "Underline (_)");

    if (config->cursor_shape) {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(ctx->combo_cursor), config->cursor_shape);
    } else {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(ctx->combo_cursor), "block");
    }

    gtk_grid_attach(GTK_GRID(grid), lbl_cursor, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ctx->combo_cursor, 1, row, 1, 1);
    row++;

    /* 5. Cursor Blink */
    GtkWidget *lbl_blink = gtk_label_new("Cursor blink:");
    gtk_widget_set_halign(lbl_blink, GTK_ALIGN_START);
    ctx->chk_blink = gtk_check_button_new_with_label("Blink cursor");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctx->chk_blink), config->cursor_blink);

    gtk_grid_attach(GTK_GRID(grid), lbl_blink, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ctx->chk_blink, 1, row, 1, 1);
    row++;

    gtk_box_pack_start(GTK_BOX(content), grid, TRUE, TRUE, 0);
    gtk_widget_show_all(content);

    g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), ctx);
    g_signal_connect(dialog, "destroy", G_CALLBACK(on_dialog_destroy), ctx);

    gtk_widget_show_all(dialog);
}
