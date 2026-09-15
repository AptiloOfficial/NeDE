#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *image;
    GtkWidget *status_label;
    char *screenshot_path;
} AppData;

static void take_screenshot(const char *filename) {
    char command[512];
    snprintf(command, sizeof(command), 
             "grim -t png '%s' 2>/dev/null || "
             "slurp -o | grim -g - '%s' 2>/dev/null || "
             "import -window root '%s' 2>/dev/null || "
             "scrot '%s' 2>/dev/null", 
             filename, filename, filename, filename);
    system(command);
}

static void update_preview(AppData *app) {
    if (app->image && app->screenshot_path) {
        GtkWidget *new_image = gtk_image_new_from_file(app->screenshot_path);
        gtk_image_set_pixel_size(GTK_IMAGE(new_image), 380);
        GtkWidget *parent = gtk_widget_get_parent(app->image);
        gtk_container_remove(GTK_CONTAINER(parent), app->image);
        gtk_box_pack_start(GTK_BOX(parent), new_image, TRUE, TRUE, 0);
        gtk_widget_show(new_image);
        app->image = new_image;
    }
}

static void on_save_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    AppData *app = (AppData*)data;
    
    if (!app->screenshot_path) return;
    
    char *home = getenv("HOME");
    char dest_path[1024];
    
    // Сохраняем прямо в ~/Pictures/
    char pics_dir[512];
    snprintf(pics_dir, sizeof(pics_dir), "%s/", home);
    mkdir(pics_dir, 0755);
    
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", tm);
    
    snprintf(dest_path, sizeof(dest_path), "%s/screenshot_%s.png", pics_dir, timestamp);
    
    GFile *src = g_file_new_for_path(app->screenshot_path);
    GFile *dest = g_file_new_for_path(dest_path);
    g_file_copy(src, dest, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
    g_object_unref(src);
    g_object_unref(dest);
    
    gtk_label_set_text(GTK_LABEL(app->status_label), "v Saved...");
    printf("Saved in: %s\n", dest_path);
}

static void on_close_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    AppData *app = (AppData*)data;
    if (app->screenshot_path) {
        g_free(app->screenshot_path);
        app->screenshot_path = NULL;
    }
    gtk_widget_destroy(app->window);
    gtk_main_quit();
}

static void on_copy_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    AppData *app = (AppData*)data;
    if (!app->screenshot_path) return;
    
    char command[256];
    snprintf(command, sizeof(command), "wl-copy < '%s' 2>/dev/null", app->screenshot_path);
    system(command);
    gtk_label_set_text(GTK_LABEL(app->status_label), "Copyed...");
}

static void on_retake_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    AppData *app = (AppData*)data;
    if (app->screenshot_path) {
        take_screenshot(app->screenshot_path);
        update_preview(app);
        gtk_label_set_text(GTK_LABEL(app->status_label), "New ScreenShot...");
    }
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    
    AppData app = {0};
    
    const char *temp_dir = g_get_tmp_dir();
    app.screenshot_path = g_build_filename(temp_dir, "nedeshot_temp.png", NULL);
    
    take_screenshot(app.screenshot_path);
    
    if (access(app.screenshot_path, F_OK) != 0) {
        g_printerr("Can't make screenshot\n");
        g_free(app.screenshot_path);
        return 1;
    }
    
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "NeDE Shot");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 420, 520);
    gtk_window_set_position(GTK_WINDOW(app.window), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name(GTK_WINDOW(app.window), "camera-photo");
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(app.window), vbox);
    
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    GtkWidget *retake_btn = gtk_button_new_with_label("O Shot Again");
    g_signal_connect(retake_btn, "clicked", G_CALLBACK(on_retake_clicked), &app);
    gtk_box_pack_start(GTK_BOX(hbox), retake_btn, TRUE, TRUE, 0);
    
    GtkWidget *save_btn = gtk_button_new_with_label("/ Save");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_clicked), &app);
    gtk_box_pack_start(GTK_BOX(hbox), save_btn, TRUE, TRUE, 0);
    
    GtkWidget *copy_btn = gtk_button_new_with_label("> Copy");
    g_signal_connect(copy_btn, "clicked", G_CALLBACK(on_copy_clicked), &app);
    gtk_box_pack_start(GTK_BOX(hbox), copy_btn, TRUE, TRUE, 0);
    
    GtkWidget *close_btn = gtk_button_new_with_label("X Close");
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), &app);
    gtk_box_pack_start(GTK_BOX(hbox), close_btn, TRUE, TRUE, 0);
    
    app.image = gtk_image_new_from_file(app.screenshot_path);
    gtk_image_set_pixel_size(GTK_IMAGE(app.image), 380);
    gtk_box_pack_start(GTK_BOX(vbox), app.image, TRUE, TRUE, 0);
    
    app.status_label = gtk_label_new("ScreenShot saved.�");
    gtk_box_pack_start(GTK_BOX(vbox), app.status_label, FALSE, FALSE, 0);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        "window {"
        "   background-color: #1a1a2e;"
        "}"
        "button {"
        "   background-color: #2a2a4a;"
        "   color: #cdd6f4;"
        "   border: none;"
        "   border-radius: 8px;"
        "   padding: 8px 16px;"
        "}"
        "button:hover {"
        "   background-color: #3a3a5a;"
        "}"
        "label {"
        "   color: #cdd6f4;"
        "}";
    
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    gtk_widget_show_all(app.window);
    gtk_main();
    
    return 0;
}

