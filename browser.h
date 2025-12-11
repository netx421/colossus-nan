// browser.h — COLOSSUS Terminal Browser

#ifndef COLOSSUS_BROWSER_H
#define COLOSSUS_BROWSER_H

#include <string>
#include <vector>

extern "C" {
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <jsc/jsc.h>
}

class Browser {
public:
    explicit Browser(GtkApplication* app);
    ~Browser();

    void show();
    void open_uri(const std::string& uri);

private:
    struct Tab {
        GtkWidget* scrolled = nullptr;
        WebKitWebView* webview = nullptr;
        GtkWidget* label = nullptr;
    };

    GtkApplication* app_ = nullptr;
    GtkWidget* window_ = nullptr;
    GtkWidget* notebook_ = nullptr;
    GtkWidget* bottom_bar_ = nullptr;
    GtkWidget* url_entry_ = nullptr;
    GtkWidget* new_tab_button_ = nullptr;
    GtkWidget* back_button_ = nullptr;
    GtkWidget* forward_button_ = nullptr;
    GtkWidget* home_button_ = nullptr;

    std::vector<Tab> tabs_;
    int current_tab_ = -1;

    std::string script_source_;

    // UI setup
    void setup_ui();
    void apply_shell_theme();
    Tab& create_tab(const std::string& uri);
    WebKitUserContentManager* create_content_manager();
    void inject_user_script(WebKitUserContentManager* manager);
    WebKitWebView* current_webview();
    Tab* get_tab_for_webview(WebKitWebView* view);

    // Navigation / loading
    void load_homepage();
    void load_uri(const std::string& uri);
    void update_url_entry_for(WebKitWebView* view);
    void update_tab_title_for(WebKitWebView* view);

    // Actions
    void new_tab(const std::string& uri);
    void go_back();
    void go_forward();
    void go_home();
    void reload();

    // Event handlers (instance)
    void on_url_entry_activate();
    void on_load_changed(WebKitWebView* view, WebKitLoadEvent event);
    void on_uri_changed(WebKitWebView* view);
    void on_title_changed(WebKitWebView* view);
    void on_tab_switched(guint page_num);
    gboolean on_key_press(GdkEventKey* event);

    void on_mpv_message(WebKitJavascriptResult* js_result);
    void on_xterm_message(WebKitJavascriptResult* js_result);

    // Helpers
    void launch_mpv(const std::string& url);
    void launch_xterm(const std::string& target);
    static std::string load_text_file(const std::string& path);

    // Static trampolines
    static void s_load_changed(WebKitWebView* webview,
                               WebKitLoadEvent load_event,
                               gpointer user_data);
    static void s_uri_changed(WebKitWebView* webview,
                              GParamSpec*,
                              gpointer user_data);
    static void s_title_changed(WebKitWebView* webview,
                                GParamSpec*,
                                gpointer user_data);
    static void s_tab_switched(GtkNotebook* notebook,
                               GtkWidget* page,
                               guint page_num,
                               gpointer user_data);
    static gboolean s_key_press(GtkWidget* widget,
                                GdkEventKey* event,
                                gpointer user_data);
    static void s_url_entry_activate(GtkEntry* entry,
                                     gpointer user_data);

    static void s_mpv_message(WebKitUserContentManager* manager,
                              WebKitJavascriptResult* result,
                              gpointer user_data);
    static void s_xterm_message(WebKitUserContentManager* manager,
                                WebKitJavascriptResult* result,
                                gpointer user_data);
};

#endif // COLOSSUS_BROWSER_H

