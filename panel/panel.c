#include <gtk/gtk.h>
#include <gtk-layer-shell.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <glib.h>
#include <math.h>

#define PANEL_HEIGHT 28
#define MAX_APPS_DISPLAY 30

typedef struct {
    GtkWidget *time_label;
    GtkWidget *power_menu;
    GtkWidget *apps_menu;
    GtkWidget *task_list;
    GtkWidget *volume_label;
    GtkWidget *battery_label;
    GtkWidget *network_label;
    GtkWidget *volume_popup;
    GtkWidget *battery_popup;
    GtkWidget *network_popup;
    GtkWidget *calendar_popup;
} AppWidgets;

typedef struct {
    char *name;
    char *exec;
    char *icon;
} App;

static GList *app_list = NULL;
static char *current_theme = NULL;
static AppWidgets *global_widgets = NULL;

// ============ UTILITY FUNCTIONS ============
static void free_app_list(void) {
    for (GList *l = app_list; l; l = l->next) {
        App *app = (App*)l->data;
        g_free(app->name);
        g_free(app->exec);
        g_free(app->icon);
        g_free(app);
    }
    g_list_free(app_list);
    app_list = NULL;
}

// ============ POPUP DESTROY HANDLER ============
static void on_popup_destroy(GtkWidget *widget, AppWidgets *widgets) {
    if (widget == widgets->volume_popup)    widgets->volume_popup = NULL;
    if (widget == widgets->battery_popup)   widgets->battery_popup = NULL;
    if (widget == widgets->network_popup)   widgets->network_popup = NULL;
    if (widget == widgets->calendar_popup)  widgets->calendar_popup = NULL;
    if (widget == widgets->apps_menu)       widgets->apps_menu = NULL;
    if (widget == widgets->power_menu)      widgets->power_menu = NULL;
}

// ============ LOAD APPLICATIONS ============
static void load_applications(void) {
    free_app_list();

    const char *dirs[] = {
        "/usr/share/applications/",
        "/usr/local/share/applications/",
        NULL
    };
    
    char *home_dir = g_strdup_printf("%s/.local/share/applications/", g_getenv("HOME"));
    const char *all_dirs[] = {dirs[0], dirs[1], home_dir, NULL};
    
    for (int d = 0; all_dirs[d]; d++) {
        DIR *dir = opendir(all_dirs[d]);
        if (!dir) continue;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (g_str_has_suffix(entry->d_name, ".desktop")) {
                char *path = g_strdup_printf("%s/%s", all_dirs[d], entry->d_name);
                GKeyFile *keyfile = g_key_file_new();
                
                if (g_key_file_load_from_file(keyfile, path, G_KEY_FILE_NONE, NULL)) {
                    gboolean hidden = g_key_file_get_boolean(keyfile, "Desktop Entry", "Hidden", NULL);
                    if (hidden) {
                        g_key_file_free(keyfile);
                        g_free(path);
                        continue;
                    }
                    
                    char *name = g_key_file_get_string(keyfile, "Desktop Entry", "Name", NULL);
                    char *exec = g_key_file_get_string(keyfile, "Desktop Entry", "Exec", NULL);
                    char *icon = g_key_file_get_string(keyfile, "Desktop Entry", "Icon", NULL);
                    char *type = g_key_file_get_string(keyfile, "Desktop Entry", "Type", NULL);
                    
                    if (name && exec && type && g_strcmp0(type, "Application") == 0) {
                        char *clean_exec = g_strdup(exec);
                        char *patterns[] = {"%U", "%u", "%F", "%f", "%D", "%d", "%N", "%n", "%c", "%k", "%v", "%m", NULL};
                        for (int i = 0; patterns[i]; i++) {
                            char *pos = strstr(clean_exec, patterns[i]);
                            if (pos) {
                                memset(pos, ' ', strlen(patterns[i]));
                            }
                        }
                        g_strstrip(clean_exec);
                        
                        App *app = g_new(App, 1);
                        app->name = g_strdup(name);
                        app->exec = g_strdup(clean_exec);
                        app->icon = icon ? g_strdup(icon) : NULL;
                        app_list = g_list_append(app_list, app);
                        g_free(clean_exec);
                    }
                    g_free(name); g_free(exec); g_free(icon); g_free(type);
                }
                g_key_file_free(keyfile);
                g_free(path);
            }
        }
        closedir(dir);
    }
    g_free(home_dir);
    app_list = g_list_sort(app_list, (GCompareFunc)strcmp);
}

// ============ GET THEME ============
static const char* get_theme(void) {
    if (!current_theme) {
        current_theme = getenv("NEDEPANEL_THEME");
        if (!current_theme) current_theme = "nedepanel";
    }
    return current_theme;
}

// ============ CSS — использует переменные GTK-темы ============
static const char* get_panel_css(void) {
    return 
        "window, button, label, #start-button, #task-button, #time-button, .indicator-button {"
        "   font-family: inherit;"
        "   font-size: inherit;"
        "}"
        "window {"
        "   background-color: @theme_bg_color;"
        "   border-top: 1px solid alpha(@theme_fg_color, 0.1);"
        "}"
        "button {"
        "   background-color: transparent;"
        "   color: @theme_fg_color;"
        "   border: none;"
        "   padding: 2px 4px;"
        "   border-radius: 0;"
        "}"
        "button:hover {"
        "   background-color: alpha(@theme_fg_color, 0.05);"
        "}"
        "#start-button {"
        "   font-size: 14px;"
        "   padding: 1px 8px;"
        "   min-width: 24px;"
        "}"
        "#power-button {"
        "   font-size: 13px;"
        "   padding: 1px 6px;"
        "   min-width: 24px;"
        "}"
        "#task-button {"
        "   padding: 1px 6px;"
        "   border-radius: 4px;"
        "   min-height: 20px;"
        "   min-width: 20px;"
        "}"
        "#task-button:hover {"
        "   background-color: alpha(@theme_fg_color, 0.08);"
        "}"
        "#task-button.active {"
        "   background-color: alpha(@theme_fg_color, 0.15);"
        "}"
        ".indicator-button {"
        "   padding: 1px 4px;"
        "   font-size: 11px;"
        "   min-width: 20px;"
        "}"
        ".indicator-button:hover {"
        "   background-color: alpha(@theme_fg_color, 0.08);"
        "}"
        "#time-button {"
        "   padding: 2px 6px;"
        "   font-size: 12px;"
        "   border-radius: 4px;"
        "}"
        "#time-button:hover {"
        "   background-color: alpha(@theme_fg_color, 0.05);"
        "}"
        "label {"
        "   color: @theme_fg_color;"
        "   font-size: 11px;"
        "}";
}

// ============ CSS для POPUP ============
static const char* get_popup_css(void) {
    return 
        "window {"
        "   background-color: @theme_bg_color;"
        "   border: 1px solid alpha(@theme_fg_color, 0.2);"
        "   border-radius: 12px;"
        "   padding: 6px;"
        "}"
        "label {"
        "   color: @theme_fg_color;"
        "   font-size: 14px;"
        "}"
        "scale {"
        "   background-color: alpha(@theme_fg_color, 0.1);"
        "   border-radius: 4px;"
        "}"
        "button {"
        "   background-color: transparent;"
        "   color: @theme_fg_color;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 4px 8px;"
        "}"
        "button:hover {"
        "   background-color: alpha(@theme_fg_color, 0.1);"
        "}"
        "calendar {"
        "   background-color: @theme_base_color;"
        "   color: @theme_fg_color;"
        "}"
        "calendar.header {"
        "   background-color: @theme_bg_color;"
        "   color: @theme_selected_bg_color;"
        "}"
        "calendar:selected {"
        "   background-color: @theme_selected_bg_color;"
        "   color: @theme_selected_fg_color;"
        "}";
}

// ============ APPLY CSS ============
static void apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;
    gtk_css_provider_load_from_data(provider, get_panel_css(), -1, &error);
    if (error) {
        g_printerr("CSS Error: %s\n", error->message);
        g_error_free(error);
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(), 
        GTK_STYLE_PROVIDER(provider), 
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

// ============ POSITIONING HELPER ============
static void position_popup_above_button(GtkWidget *popup, GtkWidget *button, int popup_width, int popup_height) {
    GdkRectangle rect;
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_monitor(display, 0);
    gdk_monitor_get_geometry(monitor, &rect);
    
    GdkWindow *window = gtk_widget_get_window(button);
    if (!window) {
        int x = rect.x + 10;
        int y = rect.y + rect.height - PANEL_HEIGHT - popup_height - 5;
        gtk_window_move(GTK_WINDOW(popup), x, y);
        return;
    }
    
    int x, y;
    gdk_window_get_origin(window, &x, &y);
    
    GtkAllocation alloc;
    gtk_widget_get_allocation(button, &alloc);
    int button_width = alloc.width;
    
    int popup_x = x + (button_width / 2) - (popup_width / 2);
    int popup_y = y - popup_height - 2;
    
    if (popup_x < rect.x) popup_x = rect.x + 5;
    if (popup_x + popup_width > rect.x + rect.width) popup_x = rect.x + rect.width - popup_width - 5;
    if (popup_y < rect.y) popup_y = rect.y + 5;
    
    gtk_window_move(GTK_WINDOW(popup), popup_x, popup_y);
}

// ============ APPS MENU (LAUNCHER) ============
static void create_apps_menu(GtkWidget *button, AppWidgets *widgets) {
    (void)button;
    if (widgets->apps_menu) {
        gtk_widget_destroy(widgets->apps_menu);
        widgets->apps_menu = NULL;
        return;
    }
    system("nedelauncher 2>/dev/null &");
}

// ============ VOLUME INDICATOR ============
static void get_volume(char *buffer, size_t size) {
    FILE *fp = popen("amixer get Master 2>/dev/null | grep -o '[0-9]*%' | head -1", "r");
    if (fp) {
        if (fgets(buffer, size, fp)) {
            buffer[strcspn(buffer, "\n")] = '\0';
        } else {
            snprintf(buffer, size, "%s", "0%");
        }
        pclose(fp);
    } else {
        snprintf(buffer, size, "%s", "0%");
    }
}

static void on_volume_changed(GtkRange *range, gpointer data) {
    (void)data;
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "amixer set Master %d%% 2>/dev/null", (int)gtk_range_get_value(range));
    system(cmd);
    
    if (global_widgets && global_widgets->volume_label) {
        char vol_str[16];
        get_volume(vol_str, sizeof(vol_str));
        char display[32];
        snprintf(display, sizeof(display), "%s", vol_str);
        gtk_label_set_text(GTK_LABEL(global_widgets->volume_label), display);
    }
}

static void on_mute_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    system("amixer set Master toggle 2>/dev/null");
    if (global_widgets && global_widgets->volume_label) {
        char vol_str[16];
        get_volume(vol_str, sizeof(vol_str));
        char display[32];
        snprintf(display, sizeof(display), "%s", vol_str);
        gtk_label_set_text(GTK_LABEL(global_widgets->volume_label), display);
    }
}

// ============ VOLUME POPUP ============
static void show_volume_popup(GtkWidget *button, AppWidgets *widgets) {
    (void)button;
    if (widgets->volume_popup) {
        gtk_widget_destroy(widgets->volume_popup);
        widgets->volume_popup = NULL;
        return;
    }
    
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_decorated(GTK_WINDOW(popup), FALSE);
    gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_NONE);
    gtk_window_set_resizable(GTK_WINDOW(popup), FALSE);
    gtk_widget_set_size_request(popup, 200, 100);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_container_add(GTK_CONTAINER(popup), box);
    
    char vol_str[16];
    get_volume(vol_str, sizeof(vol_str));
    GtkWidget *label = gtk_label_new(vol_str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    
    global_widgets = widgets;
    
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 5);
    gtk_range_set_value(GTK_RANGE(scale), atoi(vol_str));
    gtk_widget_set_size_request(scale, 180, 20);
    g_signal_connect(scale, "value-changed", G_CALLBACK(on_volume_changed), NULL);
    gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
    
    GtkWidget *mute_btn = gtk_button_new_with_label("Mute");
    gtk_box_pack_start(GTK_BOX(box), mute_btn, FALSE, FALSE, 0);
    g_signal_connect(mute_btn, "clicked", G_CALLBACK(on_mute_clicked), NULL);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, get_popup_css(), -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    gtk_widget_show_all(popup);
    g_signal_connect(popup, "focus-out-event", G_CALLBACK(gtk_widget_destroy), NULL);
    g_signal_connect(popup, "destroy", G_CALLBACK(on_popup_destroy), widgets);
    
    position_popup_above_button(popup, button, 200, 100);
    
    widgets->volume_popup = popup;
}

// ============ BATTERY INDICATOR ============
static void get_battery(char *buffer, size_t size) {
    if (system("test -d /sys/class/power_supply/BAT0 2>/dev/null") != 0 &&
        system("test -d /sys/class/power_supply/BAT1 2>/dev/null") != 0) {
	snprintf(buffer, size, "%s", "||| --%");   
    return;
    }
    
    FILE *fp = popen("cat /sys/class/power_supply/BAT0/capacity 2>/dev/null", "r");
    if (!fp) {
        fp = popen("cat /sys/class/power_supply/BAT1/capacity 2>/dev/null", "r");
    }
    if (fp) {
        char cap[16];
        if (fgets(cap, sizeof(cap), fp)) {
            cap[strcspn(cap, "\n")] = '\0';
            int percent = atoi(cap);
            snprintf(buffer, size, "||| %d%s", percent, "%");
        } else {
            snprintf(buffer, size, "%s", "||| --%");
        }
        pclose(fp);
    } else {
        snprintf(buffer, size, "%s", "||| --%");
    }
}

static void show_battery_popup(GtkWidget *button, AppWidgets *widgets) {
    (void)button;
    if (widgets->battery_popup) {
        gtk_widget_destroy(widgets->battery_popup);
        widgets->battery_popup = NULL;
        return;
    }
    
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_decorated(GTK_WINDOW(popup), FALSE);
    gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_NONE);
    gtk_window_set_resizable(GTK_WINDOW(popup), FALSE);
    gtk_widget_set_size_request(popup, 200, 60);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 15);
    gtk_container_add(GTK_CONTAINER(popup), box);
    
    char bat_str[64];
    get_battery(bat_str, sizeof(bat_str));
    GtkWidget *label = gtk_label_new(bat_str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, get_popup_css(), -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    gtk_widget_show_all(popup);
    g_signal_connect(popup, "focus-out-event", G_CALLBACK(gtk_widget_destroy), NULL);
    g_signal_connect(popup, "destroy", G_CALLBACK(on_popup_destroy), widgets);
    
    position_popup_above_button(popup, button, 200, 60);
    
    widgets->battery_popup = popup;
}

// ============ NETWORK INDICATOR ============
static void get_network(char *buffer, size_t size) {
    // WiFi через nmcli
    FILE *fp = popen("nmcli -t -f active,ssid dev wifi 2>/dev/null | grep '^yes' | cut -d: -f2", "r");
    if (fp) {
        char ssid[128];
        if (fgets(ssid, sizeof(ssid), fp)) {
            ssid[strcspn(ssid, "\n")] = '\0';
            if (strlen(ssid) > 0) {
                snprintf(buffer, size, "* %s", ssid);
                pclose(fp);
                return;
            }
        }
        pclose(fp);
    }
    
    // Ethernet
    if (system("cat /sys/class/net/eth0/operstate 2>/dev/null | grep -q up") == 0 ||
        system("cat /sys/class/net/enp*/operstate 2>/dev/null | grep -q up") == 0) {
        snprintf(buffer, size, "*|*");
        return;
    }
    
    // Нет сети
    snprintf(buffer, size, "x");
}

static void show_network_popup(GtkWidget *button, AppWidgets *widgets) {
    (void)button;
    if (widgets->network_popup) {
        gtk_widget_destroy(widgets->network_popup);
        widgets->network_popup = NULL;
        return;
    }
    
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_decorated(GTK_WINDOW(popup), FALSE);
    gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_NONE);
    gtk_window_set_resizable(GTK_WINDOW(popup), FALSE);
    gtk_widget_set_size_request(popup, 200, 60);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(box), 15);
    gtk_container_add(GTK_CONTAINER(popup), box);
    
    char net_str[64];
    get_network(net_str, sizeof(net_str));
    GtkWidget *label = gtk_label_new(net_str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, get_popup_css(), -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    gtk_widget_show_all(popup);
    g_signal_connect(popup, "focus-out-event", G_CALLBACK(gtk_widget_destroy), NULL);
    g_signal_connect(popup, "destroy", G_CALLBACK(on_popup_destroy), widgets);
    
    position_popup_above_button(popup, button, 200, 60);
    
    widgets->network_popup = popup;
}

// ============ UPDATE INDICATORS ============
static gboolean update_indicators(gpointer user_data) {
    AppWidgets *widgets = (AppWidgets*)user_data;

    if (widgets->volume_label) {
    char vol_str[16];
    get_volume(vol_str, sizeof(vol_str));
    char display[64];   // было 32
    snprintf(display, sizeof(display), "Vol: %s", vol_str);
    gtk_label_set_text(GTK_LABEL(widgets->volume_label), display);
}

    if (widgets->battery_label) {
        char bat_str[64];
        get_battery(bat_str, sizeof(bat_str));
        gtk_label_set_text(GTK_LABEL(widgets->battery_label), bat_str);
    }
    
    if (widgets->network_label) {
        char net_str[256];
        get_network(net_str, sizeof(net_str));
        gtk_label_set_text(GTK_LABEL(widgets->network_label), net_str);
    }
    
    return TRUE;
}

// ============ UPDATE WINDOW LIST ============
static void on_window_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    if (!data) return;
    const char *window_id = (const char*)data;
    char command[256];
    snprintf(command, sizeof(command), "WAYLAND_DISPLAY=wayland-0 wlrctl window focus '%s' 2>/dev/null", window_id);
    system(command);
}

static void free_string_data(gpointer data, GClosure *closure) {
    (void)closure;
    g_free(data);
}

static void update_window_list(AppWidgets *widgets) {
    if (!widgets->task_list) return;
    
    GList *children = gtk_container_get_children(GTK_CONTAINER(widgets->task_list));
    for (GList *l = children; l; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);
    
    FILE *fp = popen("WAYLAND_DISPLAY=wayland-0 wlrctl window list 2>/dev/null", "r");
    if (!fp) {
        GtkWidget *label = gtk_label_new("");
        gtk_box_pack_start(GTK_BOX(widgets->task_list), label, FALSE, FALSE, 4);
        gtk_widget_show_all(label);
        return;
    }
    
    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';
        if (strstr(line, "NedePanel") || strstr(line, "nedepanel")) continue;
        
        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        char *app_id = line;
        char *title = colon + 2;
        
        GtkWidget *btn = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
        gtk_widget_set_name(btn, "task-button");
        gtk_widget_set_tooltip_text(btn, title);
        
        GtkWidget *icon = gtk_image_new_from_icon_name(app_id, GTK_ICON_SIZE_MENU);
        if (!icon) icon = gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_MENU);
        gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
        gtk_container_add(GTK_CONTAINER(btn), icon);
        
        char *window_id = g_strdup(app_id);
	g_signal_connect_data(btn, "clicked",
                      G_CALLBACK(on_window_clicked),
                      window_id,
                      free_string_data,
                      0);
        gtk_box_pack_start(GTK_BOX(widgets->task_list), btn, FALSE, FALSE, 2);
        gtk_widget_show_all(btn);
        count++;
    }
    pclose(fp);
    
    if (count == 0) {
        GtkWidget *label = gtk_label_new("");
        gtk_widget_set_name(label, "task-empty");
        gtk_box_pack_start(GTK_BOX(widgets->task_list), label, FALSE, FALSE, 4);
        gtk_widget_show_all(label);
    }
}

static gboolean update_window_list_cb(gpointer user_data) {
    AppWidgets *widgets = (AppWidgets*)user_data;
    update_window_list(widgets);
    return TRUE;
}

// ============ TIME ============
static gboolean update_time(gpointer user_data) {
    AppWidgets *widgets = (AppWidgets*)user_data;
    time_t raw_time;
    struct tm *time_info;
    char time_str[64];
    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(time_str, sizeof(time_str), "%H:%M", time_info);
    gtk_label_set_text(GTK_LABEL(widgets->time_label), time_str);
    return TRUE;
}

// ============ CALENDAR ============
static void show_calendar_popup(GtkWidget *button, AppWidgets *widgets) {
    (void)button;
    
    if (widgets->calendar_popup) {
        gtk_widget_destroy(widgets->calendar_popup);
        widgets->calendar_popup = NULL;
        return;
    }
    
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_decorated(GTK_WINDOW(popup), FALSE);
    gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_NONE);
    gtk_window_set_resizable(GTK_WINDOW(popup), FALSE);
    gtk_widget_set_size_request(popup, 240, 280);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(popup), vbox);
    
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char date_str[128];
    strftime(date_str, sizeof(date_str), "%A, %d %B %Y", tm);
    
    GtkWidget *date_label = gtk_label_new(date_str);
    gtk_box_pack_start(GTK_BOX(vbox), date_label, FALSE, FALSE, 0);
    
    GtkWidget *calendar = gtk_calendar_new();
    gtk_box_pack_start(GTK_BOX(vbox), calendar, TRUE, TRUE, 0);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, get_popup_css(), -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    gtk_widget_show_all(popup);
    g_signal_connect(popup, "focus-out-event", G_CALLBACK(gtk_widget_destroy), NULL);
    g_signal_connect(popup, "destroy", G_CALLBACK(on_popup_destroy), widgets);
    
    position_popup_above_button(popup, button, 240, 280);
    
    widgets->calendar_popup = popup;
}

// ============ MAIN ============
int main(int argc, char **argv) {
    GtkWidget *window, *box, *button_start, *time_label, *time_btn;
    GtkWidget *task_list, *right_box;
    GtkWidget *volume_btn, *battery_btn, *network_btn;
    GtkWidget *volume_label, *battery_label, *network_label;
    AppWidgets widgets = {0};

    gtk_init(&argc, &argv);

    // CSS применяется ПОСЛЕ gtk_init — читает переменные GTK-темы
    apply_css();

    const char *theme = get_theme();
    g_print("NedePanel: using theme '%s'\n", theme);

    load_applications();

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "NedePanel");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, PANEL_HEIGHT);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_widget_set_size_request(window, -1, PANEL_HEIGHT);
    gtk_window_set_gravity(GTK_WINDOW(window), GDK_GRAVITY_SOUTH);

    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(window), PANEL_HEIGHT);
    gtk_layer_set_namespace(GTK_WINDOW(window), "panel");

    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_halign(box, GTK_ALIGN_FILL);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(box), 2);

    // ===== LEFT SIDE =====
    button_start = gtk_button_new_with_label("●");
    gtk_button_set_relief(GTK_BUTTON(button_start), GTK_RELIEF_NONE);
    gtk_widget_set_name(button_start, "start-button");
    g_signal_connect(button_start, "clicked", G_CALLBACK(create_apps_menu), &widgets);
    gtk_box_pack_start(GTK_BOX(box), button_start, FALSE, FALSE, 0);

    // ===== TASK LIST =====
    task_list = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_halign(task_list, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(task_list, TRUE);
    widgets.task_list = task_list;
    gtk_box_pack_start(GTK_BOX(box), task_list, TRUE, TRUE, 0);

    // ===== RIGHT SIDE =====
    right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(box), right_box, FALSE, FALSE, 0);

    // Volume
    volume_btn = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(volume_btn), GTK_RELIEF_NONE);
    gtk_widget_set_name(volume_btn, "indicator-button");
    volume_label = gtk_label_new("0%");
    gtk_container_add(GTK_CONTAINER(volume_btn), volume_label);
    widgets.volume_label = volume_label;
    g_signal_connect(volume_btn, "clicked", G_CALLBACK(show_volume_popup), &widgets);
    gtk_box_pack_start(GTK_BOX(right_box), volume_btn, FALSE, FALSE, 0);

    // Battery
    battery_btn = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(battery_btn), GTK_RELIEF_NONE);
    gtk_widget_set_name(battery_btn, "indicator-button");
    battery_label = gtk_label_new("||| --%");
    gtk_container_add(GTK_CONTAINER(battery_btn), battery_label);
    widgets.battery_label = battery_label;
    g_signal_connect(battery_btn, "clicked", G_CALLBACK(show_battery_popup), &widgets);
    gtk_box_pack_start(GTK_BOX(right_box), battery_btn, FALSE, FALSE, 0);

    // Network
    network_btn = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(network_btn), GTK_RELIEF_NONE);
    gtk_widget_set_name(network_btn, "indicator-button");
    network_label = gtk_label_new("x");
    gtk_container_add(GTK_CONTAINER(network_btn), network_label);
    widgets.network_label = network_label;
    g_signal_connect(network_btn, "clicked", G_CALLBACK(show_network_popup), &widgets);
    gtk_box_pack_start(GTK_BOX(right_box), network_btn, FALSE, FALSE, 0);

    // Time
    time_btn = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(time_btn), GTK_RELIEF_NONE);
    gtk_widget_set_name(time_btn, "time-button");
    
    time_label = gtk_label_new("--:--");
    gtk_container_add(GTK_CONTAINER(time_btn), time_label);
    
    g_signal_connect(time_btn, "clicked", G_CALLBACK(show_calendar_popup), &widgets);
    gtk_box_pack_start(GTK_BOX(right_box), time_btn, FALSE, FALSE, 4);
    widgets.time_label = time_label;

    gtk_container_add(GTK_CONTAINER(window), box);

    // Timers
    g_timeout_add_seconds(1, update_time, &widgets);
    g_timeout_add_seconds(1, update_window_list_cb, &widgets);
    g_timeout_add_seconds(5, update_indicators, &widgets);

    atexit(free_app_list);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
