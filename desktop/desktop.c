#include <gtk/gtk.h>
#include <gtk-layer-shell.h>
#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define DESKTOP_ICON_SIZE 64

typedef struct {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *trash_btn;
    GtkWidget *trash_container;
    char desktop_dir[4096];
    char trash_path[4096];
    GList *desktop_entries;
} DesktopApp;

typedef struct {
    char *name;
    char *exec;
    char *icon;
    char *path;
    time_t mtime;
} DesktopEntry;

static DesktopApp app = {0};

// ============ УТИЛИТЫ ============
static void free_desktop_entries(void) {
    for (GList *l = app.desktop_entries; l; l = l->next) {
        DesktopEntry *entry = (DesktopEntry*)l->data;
        g_free(entry->name);
        g_free(entry->exec);
        g_free(entry->icon);
        g_free(entry->path);
        g_free(entry);
    }
    g_list_free(app.desktop_entries);
    app.desktop_entries = NULL;
}

// ============ ЗАПУСК ПРИЛОЖЕНИЯ ============
static void launch_application(const char *cmd) {
    if (!cmd) return;
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        execlp("/bin/sh", "sh", "-c", cmd, NULL);
        exit(1);
    }
}

// ============ ОТКРЫТИЕ КОРЗИНЫ ============
static void open_trash(void) {
    const char *file_managers[] = {
        "thunar trash:///",
        "pcmanfm trash:///",
        "nautilus trash:///",
        "caja trash:///",
        NULL
    };
    for (int i = 0; file_managers[i]; i++) {
        char cmd[4096];
        snprintf(cmd, sizeof(cmd), "%s 2>/dev/null", file_managers[i]);
        if (system(cmd) == 0) break;
    }
}

// ============ ОЧИСТКА КОРЗИНЫ ============
static void empty_trash(void) {
GtkWidget *dialog = gtk_message_dialog_new(
    GTK_WINDOW(app.window),
    GTK_DIALOG_MODAL,
    GTK_MESSAGE_QUESTION,
    GTK_BUTTONS_YES_NO,
    "Clear trash?"
);
gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
    "Think twice, you won't be able to recover data.");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
        system("gio trash --empty 2>/dev/null");
        gtk_widget_queue_draw(app.window);
    }
    gtk_widget_destroy(dialog);
}

// ============ ОБНОВЛЕНИЕ СЕТКИ ============
static void refresh_desktop_grid(void);

// ============ КОЛБЭКИ ДЛЯ МЕНЮ ============
static void on_create_folder(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    char path[4096];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d_%H-%M-%S", tm);
    snprintf(path, sizeof(path), "%s/Folder_%s", app.desktop_dir, date_str);
    mkdir(path, 0755);
    refresh_desktop_grid();
}

static void on_refresh_desktop(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    refresh_desktop_grid();
}

// ============ ПОЛУЧЕНИЕ ИКОНКИ ============
static GtkWidget* get_icon_widget(const char *icon_name, int size) {
    GtkWidget *icon;
    if (icon_name && strlen(icon_name) > 0) {
        if (g_path_is_absolute(icon_name) && g_file_test(icon_name, G_FILE_TEST_EXISTS)) {
            icon = gtk_image_new_from_file(icon_name);
        } else {
            icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_DIALOG);
        }
    } else {
        icon = gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_DIALOG);
    }
    gtk_image_set_pixel_size(GTK_IMAGE(icon), size);
    return icon;
}

// ============ СОЗДАНИЕ КНОПКИ ПРИЛОЖЕНИЯ ============
static GtkWidget* create_app_button(DesktopEntry *entry) {
    GtkWidget *btn = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
    gtk_widget_set_size_request(btn, 90, 100);
    gtk_widget_set_tooltip_text(btn, entry->name);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(btn), vbox);
    
    GtkWidget *icon = get_icon_widget(entry->icon, DESKTOP_ICON_SIZE);
    gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), icon, FALSE, FALSE, 0);
    
    char display_name[64];
    if (strlen(entry->name) > 14) {
        strncpy(display_name, entry->name, 12);
        display_name[12] = '\0';
        strcat(display_name, "...");
    } else {
        strcpy(display_name, entry->name);
    }
    
    GtkWidget *label = gtk_label_new(display_name);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 10);
    gtk_label_set_lines(GTK_LABEL(label), 2);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    if (entry->exec) {
        g_signal_connect_swapped(btn, "clicked", G_CALLBACK(launch_application), 
                                 (gpointer)g_strdup(entry->exec));
    }
    
    return btn;
}

// ============ ЗАГРУЗКА .DESKTOP ФАЙЛОВ ============
static void load_desktop_entries(void) {
    free_desktop_entries();
    
    DIR *dir = opendir(app.desktop_dir);
    if (!dir) {
        mkdir(app.desktop_dir, 0755);
        dir = opendir(app.desktop_dir);
        if (!dir) return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!g_str_has_suffix(entry->d_name, ".desktop")) continue;
        
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", app.desktop_dir, entry->d_name);
        
        GKeyFile *keyfile = g_key_file_new();
        if (!g_key_file_load_from_file(keyfile, path, G_KEY_FILE_NONE, NULL)) {
            g_key_file_free(keyfile);
            continue;
        }
        
        char *name = g_key_file_get_string(keyfile, "Desktop Entry", "Name", NULL);
        char *exec = g_key_file_get_string(keyfile, "Desktop Entry", "Exec", NULL);
        char *icon = g_key_file_get_string(keyfile, "Desktop Entry", "Icon", NULL);
        char *type = g_key_file_get_string(keyfile, "Desktop Entry", "Type", NULL);
        gboolean hidden = g_key_file_get_boolean(keyfile, "Desktop Entry", "Hidden", NULL);
        
        struct stat st;
        time_t mtime = 0;
        if (stat(path, &st) == 0) {
            mtime = st.st_mtime;
        }
        
        if (name && exec && type && g_strcmp0(type, "Application") == 0 && !hidden) {
            char *clean_exec = g_strdup(exec);
            const char *patterns[] = {"%U", "%u", "%F", "%f", "%D", "%d", "%N", "%n", "%c", "%k", "%v", "%m", NULL};
            for (int i = 0; patterns[i]; i++) {
                char *pos = strstr(clean_exec, patterns[i]);
                if (pos) {
                    memset(pos, ' ', strlen(patterns[i]));
                }
            }
            g_strstrip(clean_exec);
            
            DesktopEntry *de = g_new0(DesktopEntry, 1);
            de->name = g_strdup(name);
            de->exec = g_strdup(clean_exec);
            de->icon = icon ? g_strdup(icon) : NULL;
            de->path = g_strdup(path);
            de->mtime = mtime;
            app.desktop_entries = g_list_append(app.desktop_entries, de);
            
            g_free(clean_exec);
        }
        
        g_free(name);
        g_free(exec);
        g_free(icon);
        g_free(type);
        g_key_file_free(keyfile);
    }
    closedir(dir);
    
    app.desktop_entries = g_list_sort(app.desktop_entries, 
        (GCompareFunc)strcmp);
}

// ============ ОБНОВЛЕНИЕ СЕТКИ ============
static void refresh_desktop_grid(void) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(app.grid));
    for (GList *l = children; l; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);
    
    load_desktop_entries();
    
    int row = 0, col = 0;
    int max_cols = 4;
    
    // Добавляем иконки приложений
    for (GList *l = app.desktop_entries; l; l = l->next) {
        DesktopEntry *de = (DesktopEntry*)l->data;
        GtkWidget *btn = create_app_button(de);
        gtk_widget_set_halign(btn, GTK_ALIGN_START);
        gtk_widget_set_valign(btn, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(app.grid), btn, col, row, 1, 1);
        
        col++;
        if (col >= max_cols) {
            col = 0;
            row++;
        }
    }
    
    // Корзина — отдельный контейнер, всегда справа внизу
    if (app.trash_container) {
        gtk_widget_destroy(app.trash_container);
    }
    
    app.trash_container = gtk_fixed_new();
    gtk_widget_set_halign(app.trash_container, GTK_ALIGN_END);
    gtk_widget_set_valign(app.trash_container, GTK_ALIGN_END);
    gtk_widget_set_hexpand(app.trash_container, TRUE);
    gtk_widget_set_vexpand(app.trash_container, TRUE);
    
    // Перемещаем корзину в новый контейнер
    if (app.trash_btn) {
        GtkWidget *old_parent = gtk_widget_get_parent(app.trash_btn);
        if (old_parent) {
            gtk_container_remove(GTK_CONTAINER(old_parent), app.trash_btn);
        }
        gtk_fixed_put(GTK_FIXED(app.trash_container), app.trash_btn, 0, 0);
        gtk_widget_set_halign(app.trash_btn, GTK_ALIGN_END);
        gtk_widget_set_valign(app.trash_btn, GTK_ALIGN_END);
    }
    
    gtk_grid_attach(GTK_GRID(app.grid), app.trash_container, max_cols - 1, 0, 1, row + 2);
    gtk_widget_show_all(app.window);
}

// ============ КОРЗИНА (КНОПКА + МЕНЮ) ============
static gboolean on_trash_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    (void)widget;
    (void)data;
    if (event->button == 3) {
        GtkWidget *menu = gtk_menu_new();
        
        char cmd[4096];
        snprintf(cmd, sizeof(cmd), "find %s -type f 2>/dev/null | wc -l", app.trash_path);
        FILE *fp = popen(cmd, "r");
        char count_str[32] = "0";
        if (fp) {
            fgets(count_str, sizeof(count_str), fp);
            pclose(fp);
        }
        int count = atoi(count_str);
        
        char empty_label[64];
        snprintf(empty_label, sizeof(empty_label), "Clear bin? (%d objects)", count);
        
        GtkWidget *empty_item = gtk_menu_item_new_with_label(empty_label);
        g_signal_connect(empty_item, "activate", G_CALLBACK(empty_trash), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), empty_item);
        
        if (count == 0) {
            gtk_widget_set_sensitive(empty_item, FALSE);
        }
        
        GtkWidget *open_item = gtk_menu_item_new_with_label("Open Bin");
        g_signal_connect(open_item, "activate", G_CALLBACK(open_trash), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), open_item);
        
        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
        g_signal_connect(menu, "deactivate", G_CALLBACK(gtk_widget_destroy), NULL);
        return TRUE;
    }
    return FALSE;
}

static GtkWidget* create_trash_icon(void) {
    GtkWidget *btn = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
    gtk_widget_set_size_request(btn, 80, 90);
    gtk_widget_set_tooltip_text(btn, "Bin");
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(btn), vbox);
    
    GtkWidget *icon = gtk_label_new("🗑");
    gtk_widget_set_name(icon, "trash-icon");
    gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), icon, FALSE, FALSE, 0);
    
    GtkWidget *label = gtk_label_new("Bin");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    g_signal_connect(btn, "clicked", G_CALLBACK(open_trash), NULL);
    g_signal_connect(btn, "button-press-event", G_CALLBACK(on_trash_button_press), NULL);
    
    app.trash_btn = btn;
    return btn;
}

// ============ КОНТЕКСТНОЕ МЕНЮ РАБОЧЕГО СТОЛА ============
static void show_desktop_menu(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    (void)widget;
    (void)data;
    if (event->button != 3) return;
    
    GtkWidget *menu = gtk_menu_new();
    
    GtkWidget *folder_item = gtk_menu_item_new_with_label("📁 Create folder");
    g_signal_connect(folder_item, "activate", G_CALLBACK(on_create_folder), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), folder_item);
    
    GtkWidget *refresh_item = gtk_menu_item_new_with_label("🔄 Refresh");
    g_signal_connect(refresh_item, "activate", G_CALLBACK(on_refresh_desktop), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), refresh_item);
    
    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
    g_signal_connect(menu, "deactivate", G_CALLBACK(gtk_widget_destroy), NULL);
}

// ============ ОБРАБОТКА КЛАВИШ ============
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    (void)widget;
    (void)data;
    
    if (event->keyval == GDK_KEY_F5) {
        refresh_desktop_grid();
        return TRUE;
    }
    
    if (event->keyval == GDK_KEY_Delete) {
        empty_trash();
        return TRUE;
    }
    
    return FALSE;
}

// ============ ГЛАВНАЯ ============
int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    
    char *home = getenv("HOME");
    snprintf(app.desktop_dir, sizeof(app.desktop_dir), "%s/Desktop", home);
    snprintf(app.trash_path, sizeof(app.trash_path), "%s/.local/share/Trash", home);
    
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "NeDE Desktop");
    gtk_window_set_decorated(GTK_WINDOW(app.window), FALSE);
    gtk_window_fullscreen(GTK_WINDOW(app.window));
    gtk_window_set_keep_below(GTK_WINDOW(app.window), TRUE);
    
    gtk_layer_init_for_window(GTK_WINDOW(app.window));
    gtk_layer_set_layer(GTK_WINDOW(app.window), GTK_LAYER_SHELL_LAYER_BOTTOM);
    gtk_layer_set_anchor(GTK_WINDOW(app.window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(app.window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(app.window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(app.window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(app.window), -1);
    gtk_layer_set_namespace(GTK_WINDOW(app.window), "desktop");
    
    GtkWidget *event_box = gtk_event_box_new();
    gtk_widget_add_events(event_box, GDK_BUTTON_PRESS_MASK);
    gtk_container_add(GTK_CONTAINER(app.window), event_box);
    
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), FALSE);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), FALSE);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_top(grid, 30);
    gtk_widget_set_margin_start(grid, 30);
    gtk_widget_set_margin_bottom(grid, 30);
    gtk_widget_set_margin_end(grid, 30);
    gtk_container_add(GTK_CONTAINER(event_box), grid);
    app.grid = grid;
    
    load_desktop_entries();
    
    GtkWidget *trash = create_trash_icon();
    app.trash_btn = trash;
    app.trash_container = NULL;
    
    refresh_desktop_grid();
    
    g_signal_connect(event_box, "button-press-event", G_CALLBACK(show_desktop_menu), NULL);
    g_signal_connect(app.window, "key-press-event", G_CALLBACK(on_key_press), NULL);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        "window {"
        "   background-color: transparent;"
        "}"
        "button {"
        "   background-color: rgba(255, 255, 255, 0.0);"
        "   color: #cdd6f4;"
        "   border: none;"
        "   border-radius: 10px;"
        "   padding: 8px;"
        "   transition: all 0.15s ease;"
        "}"
        "button:hover {"
        "   background-color: rgba(255, 255, 255, 0.08);"
        "   box-shadow: 0 2px 8px rgba(0,0,0,0.3);"
        "}"
        "button:active {"
        "   background-color: rgba(255, 255, 255, 0.05);"
        "}"
        "label {"
        "   color: #cdd6f4;"
        "   font-size: 11px;"
        "   text-shadow: 0 1px 4px rgba(0,0,0,0.9);"
        "   font-weight: 400;"
        "}"
        "#trash-icon {"
        "   font-size: 36px;"
        "}";
    
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(), 
        GTK_STYLE_PROVIDER(provider), 
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
    
    gtk_widget_show_all(app.window);
    gtk_main();
    
    free_desktop_entries();
    
    return 0;
}
