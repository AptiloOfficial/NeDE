#define POLKIT_AGENT_I_KNOW_API_IS_SUBJECT_TO_CHANGE

#include <gtk/gtk.h>
#include <polkit/polkit.h>
#include <polkitagent/polkitagent.h>
#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ============ ГЛОБАЛЬНЫЕ ВИДЖЕТЫ ============
static GtkWidget *g_window = NULL;
static GtkWidget *g_message_label = NULL;
static GtkWidget *g_password_entry = NULL;
static GtkWidget *g_status_label = NULL;
static GtkWidget *g_user_label = NULL;
static GtkWidget *g_login_button = NULL;
static PolkitAgentSession *g_session = NULL;
static gchar *g_user = NULL;

// ============ ЗАКРЫТИЕ ============
static gboolean hide_window_cb(gpointer data) {
    (void)data;
    if (g_window) gtk_widget_hide(g_window);
    return FALSE;
}

// ============ СИГНАЛЫ СЕССИИ ============
static void on_request(PolkitAgentSession *session, const gchar *request, 
                       gboolean echo_on, gpointer user_data) {
    (void)session; (void)echo_on; (void)user_data;
    g_printerr("DEBUG: on_request called\n");
    if (g_message_label) gtk_label_set_text(GTK_LABEL(g_message_label), request);
    if (g_status_label)  gtk_label_set_text(GTK_LABEL(g_status_label), "Enter your password");
    if (g_password_entry) gtk_entry_set_text(GTK_ENTRY(g_password_entry), "");
    if (g_window) {
        gtk_widget_show_all(g_window);
        gtk_window_present(GTK_WINDOW(g_window));
        gtk_widget_grab_focus(g_password_entry);
    }
}

static void on_completed(PolkitAgentSession *session, gboolean gained, gpointer user_data) {
    (void)session; (void)user_data;
    g_printerr("DEBUG: on_completed, gained=%d\n", gained);
    if (gained) {
        if (g_status_label) gtk_label_set_text(GTK_LABEL(g_status_label), "✅ Success");
        g_timeout_add(800, hide_window_cb, NULL);
    } else {
        if (g_status_label) gtk_label_set_text(GTK_LABEL(g_status_label), "❌ Failed");
        if (g_password_entry) gtk_entry_set_text(GTK_ENTRY(g_password_entry), "");
        if (g_login_button) gtk_widget_set_sensitive(g_login_button, TRUE);
    }
}

static void on_show_error(PolkitAgentSession *s, const gchar *text, gpointer d) {
    (void)s; (void)d;
    if (g_status_label) gtk_label_set_text(GTK_LABEL(g_status_label), text);
}

static void on_show_info(PolkitAgentSession *s, const gchar *text, gpointer d) {
    (void)s; (void)d;
    if (g_status_label) gtk_label_set_text(GTK_LABEL(g_status_label), text);
}

// ============ КНОПКИ ============
static void on_login_clicked(GtkWidget *w, gpointer d) {
    (void)w; (void)d;
    if (!g_password_entry || !g_session) {
        g_printerr("DEBUG: g_session=%p\n", (void*)g_session);
        return;
    }
    const char *pw = gtk_entry_get_text(GTK_ENTRY(g_password_entry));
    g_printerr("DEBUG: password length=%zu, session=%p\n", strlen(pw), (void*)g_session);
    if (!pw || strlen(pw) == 0) return;
    
    if (g_login_button) gtk_widget_set_sensitive(g_login_button, FALSE);
    if (g_status_label) gtk_label_set_text(GTK_LABEL(g_status_label), "⏳ Authenticating...");
    
    polkit_agent_session_response(g_session, pw);
    g_printerr("DEBUG: polkit_agent_session_response sent\n");
}

static void on_cancel_clicked(GtkWidget *w, gpointer d) {
    (void)w; (void)d;
    if (g_session) polkit_agent_session_cancel(g_session);
    if (g_window) gtk_widget_hide(g_window);
}

static void on_activate(GtkEntry *e, gpointer d) {
    (void)e; (void)d;
    on_login_clicked(NULL, NULL);
}

// ============ СОЗДАНИЕ ОКНА ============
static void create_window(void) {
    g_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_window), "NeDE UAC");
    gtk_window_set_default_size(GTK_WINDOW(g_window), 420, 340);
    gtk_window_set_position(GTK_WINDOW(g_window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(g_window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(g_window), TRUE);
    gtk_window_set_modal(GTK_WINDOW(g_window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(g_window), FALSE);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 25);
    gtk_container_add(GTK_CONTAINER(g_window), vbox);
    
    // Заголовок
    GtkWidget *title = gtk_label_new("Authentication Required");
    gtk_widget_set_name(title, "uac-title");
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    
    // Сообщение
    g_message_label = gtk_label_new("Enter your password");
    gtk_label_set_line_wrap(GTK_LABEL(g_message_label), TRUE);
    gtk_widget_set_halign(g_message_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), g_message_label, FALSE, FALSE, 0);
    
    // Пользователь
    g_user_label = gtk_label_new(g_user);
    gtk_widget_set_name(g_user_label, "uac-user");
    gtk_widget_set_halign(g_user_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), g_user_label, FALSE, FALSE, 0);
    
    // Пароль
    g_password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(g_password_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_password_entry), "Password");
    gtk_widget_set_size_request(g_password_entry, -1, 40);
    g_signal_connect(g_password_entry, "activate", G_CALLBACK(on_activate), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), g_password_entry, FALSE, FALSE, 0);
    
    // Статус
    g_status_label = gtk_label_new("Enter your password");
    gtk_widget_set_halign(g_status_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), g_status_label, FALSE, FALSE, 0);
    
    // Кнопки
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *cancel_btn = gtk_button_new_with_label("✕ Cancel");
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_cancel_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(btn_box), cancel_btn, TRUE, TRUE, 0);
    
    g_login_button = gtk_button_new_with_label("▶ Authenticate");
    gtk_widget_set_name(g_login_button, "auth-btn");
    g_signal_connect(g_login_button, "clicked", G_CALLBACK(on_login_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(btn_box), g_login_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), btn_box, FALSE, FALSE, 0);
    
    // CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        "window { background: rgba(20,20,30,245); border: 1px solid #4a4a5a; border-radius: 16px; font-family: 'DejaVu Sans'; }"
        "#uac-title { font-size: 16px; font-weight: bold; color: #cdd6f4; }"
        "#uac-user { font-size: 14px; color: #89b4fa; font-weight: bold; }"
        "entry { background: rgba(30,30,40,200); color: #cdd6f4; border: 1px solid #4a4a5a; border-radius: 8px; padding: 8px 12px; }"
        "entry:focus { border-color: #89b4fa; }"
        "button { background: rgba(40,40,50,200); color: #cdd6f4; border: none; border-radius: 8px; padding: 8px 16px; font-weight: bold; }"
        "button:hover { background: rgba(60,60,70,200); }"
        "#auth-btn { background: #89b4fa; color: #1a1a2e; }"
        "#auth-btn:hover { background: #a6c8ff; }"
        "label { color: #cdd6f4; }";
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

// ============ LISTENER ============
typedef struct _NedeUacListener NedeUacListener;
typedef struct _NedeUacListenerClass NedeUacListenerClass;
struct _NedeUacListener { PolkitAgentListener parent_instance; };
struct _NedeUacListenerClass { PolkitAgentListenerClass parent_class; };

G_DEFINE_TYPE(NedeUacListener, nede_uac_listener, POLKIT_AGENT_TYPE_LISTENER)

static void nede_uac_listener_initiate_authentication(
        PolkitAgentListener  *listener,
        const gchar          *action_id,
        const gchar          *message,
        const gchar          *icon_name,
        PolkitDetails        *details,
        const gchar          *cookie,
        GList                *identities,
        GCancellable         *cancellable,
        GAsyncReadyCallback   callback,
        gpointer              user_data) {
    
    (void)listener; (void)action_id; (void)icon_name; (void)details;
    
    // ЖЁСТКАЯ ОТЛАДКА — пишем в файл
    FILE *f = fopen("/tmp/nedeuac-debug.log", "a");
    if (f) {
        fprintf(f, "=== initiate_authentication called ===\n");
        fprintf(f, "message: %s\n", message ? message : "(null)");
        fprintf(f, "cookie: %s\n", cookie ? cookie : "(null)");
        fprintf(f, "identities: %p\n", (void*)identities);
        fclose(f);
    }
    
    g_printerr("DEBUG: initiate_authentication called\n");
    
    // Пользователь
    if (identities && identities->data) {
        PolkitIdentity *id = POLKIT_IDENTITY(identities->data);
        if (POLKIT_IS_UNIX_USER(id)) {
            const gchar *name = polkit_unix_user_get_name(POLKIT_UNIX_USER(id));
            if (name) {
                g_free(g_user);
                g_user = g_strdup(name);
                if (g_user_label) gtk_label_set_text(GTK_LABEL(g_user_label), g_user);
            }
        }
    }
    
	// Сессия
	PolkitIdentity *id = polkit_unix_user_new_for_name(g_user, NULL);
	if (g_session) g_object_unref(g_session);
	g_session = polkit_agent_session_new(id, cookie);
	g_object_unref(id);

	g_signal_connect(g_session, "request", G_CALLBACK(on_request), NULL);
	g_signal_connect(g_session, "completed", G_CALLBACK(on_completed), NULL);
	g_signal_connect(g_session, "show-error", G_CALLBACK(on_show_error), NULL);
	g_signal_connect(g_session, "show-info", G_CALLBACK(on_show_info), NULL);
    
    // Сообщение
    if (message && *message) {
        gtk_label_set_text(GTK_LABEL(g_message_label), message);
    }
    
        // Показываем окно
    gtk_widget_show_all(g_window);
    gtk_window_present(GTK_WINDOW(g_window));
    gtk_widget_grab_focus(g_password_entry);
    
    // Откладываем initiate на следующий цикл событий
    // чтобы GTK успел зарегистрировать все обработчики
    polkit_agent_session_initiate(g_session);
    
    // Завершаем асинхронный вызов
    GTask *task = g_task_new(listener, cancellable, callback, user_data);
    g_task_return_boolean(task, TRUE);
    g_object_unref(task);
}

static gboolean nede_uac_listener_initiate_authentication_finish(
        PolkitAgentListener *listener, GAsyncResult *res, GError **error) {
    (void)listener;
    return g_task_propagate_boolean(G_TASK(res), error);
}

static void nede_uac_listener_class_init(NedeUacListenerClass *klass) {
    PolkitAgentListenerClass *lc = POLKIT_AGENT_LISTENER_CLASS(klass);
    lc->initiate_authentication = nede_uac_listener_initiate_authentication;
    lc->initiate_authentication_finish = nede_uac_listener_initiate_authentication_finish;
}

static void nede_uac_listener_init(NedeUacListener *l) { (void)l; }
static PolkitAgentListener* nede_uac_listener_new(void) {
    return g_object_new(nede_uac_listener_get_type(), NULL);
}

// ============ ГЛАВНАЯ ============
int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    
    g_user = g_strdup(g_get_user_name());
    
    // Создаём окно заранее, но НЕ показываем
    create_window();
    
    GError *error = NULL;
    PolkitAgentListener *listener = nede_uac_listener_new();
    
    PolkitSubject *subject = polkit_unix_session_new_for_process_sync(getpid(), NULL, &error);
    if (!subject) {
        g_printerr("Failed to create subject: %s\n", error ? error->message : "?");
        if (error) g_error_free(error);
        return 1;
    }
    
    gpointer handle = polkit_agent_listener_register(listener,
                                                     POLKIT_AGENT_REGISTER_FLAGS_NONE,
                                                     subject, NULL, NULL, &error);
    if (!handle) {
        g_printerr("Failed to register: %s\n", error ? error->message : "?");
        if (error) g_error_free(error);
        return 1;
    }
    
    g_print("NeDE UAC agent registered\n");
    
    gtk_main();
    
    if (handle) polkit_agent_listener_unregister(handle);
    g_object_unref(listener);
    g_object_unref(subject);
    g_free(g_user);
    return 0;
}
