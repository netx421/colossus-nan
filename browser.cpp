// browser.cpp — COLOSSUS Terminal Browser with tabs + bottom command bar

#include "browser.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <gdk/gdkkeysyms.h>

// Default homepage
static const char* COLOSSUS_HOMEPAGE = "https://search.brave.com/";

// ───────────────────────────────────────────────
//  Utility
// ───────────────────────────────────────────────

std::string Browser::load_text_file(const std::string& path)
{
    auto try_read = [](const std::string& p) -> std::string {
        std::ifstream in(p);
        if (!in.is_open())
            return {};
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    };

    // 1) Try relative path (developer mode, run from project directory)
    std::string p1 = path;
    std::string data = try_read(p1);
    if (!data.empty())
        return data;

    // 2) Try installed system path
    std::string p2 = "/usr/local/share/colossus-nan/" + path;
    data = try_read(p2);
    if (!data.empty())
        return data;

    // 3) Report failure (helpful for debugging)
    g_printerr("COLOSSUS-NAN: Resource not found: '%s' (tried '%s' and '%s')\n",
               path.c_str(), p1.c_str(), p2.c_str());

    return {};
}


// ───────────────────────────────────────────────
//  Constructor / destructor
// ───────────────────────────────────────────────

Browser::Browser(GtkApplication* app)
    : app_(app)
{
    script_source_ = load_text_file("resources/browser.js");
    if (script_source_.empty()) {
        std::cerr << "Warning: resources/browser.js is empty or missing. "
                  << "Terminal view + MPV / Telehack integration will not work.\n";
    }

    setup_ui();
    load_homepage();
}

Browser::~Browser()
{
    if (window_) {
        gtk_widget_destroy(window_);
        window_ = nullptr;
    }
}



// ───────────────────────────────────────────────
//  UI setup
// ───────────────────────────────────────────────

void Browser::setup_ui()
{
    // Main window
    window_ = gtk_application_window_new(app_);
    gtk_window_set_default_size(GTK_WINDOW(window_), 1100, 700);
    gtk_window_set_title(GTK_WINDOW(window_), "COLOSSUS: NETWORK ACCESS NODE");

    g_signal_connect(window_, "key-press-event",
                     G_CALLBACK(Browser::s_key_press), this);

    // Vertical layout: notebook + bottom command bar
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window_), vbox);

    // Tabs notebook
    notebook_ = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook_), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), notebook_, TRUE, TRUE, 0);

    g_signal_connect(notebook_, "switch-page",
                     G_CALLBACK(Browser::s_tab_switched), this);

    // Bottom command bar (retro input line)
    bottom_bar_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(bottom_bar_, 6);
    gtk_widget_set_margin_end(bottom_bar_, 6);
    gtk_widget_set_margin_bottom(bottom_bar_, 4);
    gtk_box_pack_end(GTK_BOX(vbox), bottom_bar_, FALSE, FALSE, 0);

    // Navigation buttons
    back_button_ = gtk_button_new_with_label("<");
    forward_button_ = gtk_button_new_with_label(">");
    home_button_ = gtk_button_new_with_label("HOME");

    g_signal_connect(back_button_, "clicked",
                     G_CALLBACK(+[](GtkButton*, gpointer data) {
                         auto* self = static_cast<Browser*>(data);
                         if (self) self->go_back();
                     }),
                     this);

    g_signal_connect(forward_button_, "clicked",
                     G_CALLBACK(+[](GtkButton*, gpointer data) {
                         auto* self = static_cast<Browser*>(data);
                         if (self) self->go_forward();
                     }),
                     this);

    g_signal_connect(home_button_, "clicked",
                     G_CALLBACK(+[](GtkButton*, gpointer data) {
                         auto* self = static_cast<Browser*>(data);
                         if (self) self->go_home();
                     }),
                     this);

    gtk_box_pack_start(GTK_BOX(bottom_bar_), back_button_, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bottom_bar_), forward_button_, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bottom_bar_), home_button_, FALSE, FALSE, 4);

    // URL entry
    url_entry_ = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(url_entry_),
                                   "Enter URL or search…");
    gtk_widget_set_hexpand(url_entry_, TRUE);
    g_signal_connect(url_entry_, "activate",
                     G_CALLBACK(Browser::s_url_entry_activate), this);
    gtk_box_pack_start(GTK_BOX(bottom_bar_), url_entry_, TRUE, TRUE, 0);

    // New tab button
    new_tab_button_ = gtk_button_new_with_label("+");
    g_signal_connect(new_tab_button_, "clicked",
                     G_CALLBACK(+[](GtkButton*, gpointer data) {
                         auto* self = static_cast<Browser*>(data);
                         if (self) self->new_tab(COLOSSUS_HOMEPAGE);
                     }),
                     this);
    gtk_box_pack_start(GTK_BOX(bottom_bar_), new_tab_button_, FALSE, FALSE, 0);

    apply_shell_theme();

    // Initial tab created in load_homepage()
}

void Browser::apply_shell_theme()
{
    // Force black shell to avoid white flashes from GTK theme
    GdkRGBA black;
    black.red   = 0.0;
    black.green = 0.0;
    black.blue  = 0.0;
    black.alpha = 1.0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    if (window_)     gtk_widget_override_background_color(window_,     GTK_STATE_FLAG_NORMAL, &black);
    if (notebook_)   gtk_widget_override_background_color(notebook_,   GTK_STATE_FLAG_NORMAL, &black);
    if (bottom_bar_) gtk_widget_override_background_color(bottom_bar_, GTK_STATE_FLAG_NORMAL, &black);
#pragma GCC diagnostic pop
}

// ───────────────────────────────────────────────
//  Tabs / WebViews
// ───────────────────────────────────────────────

WebKitUserContentManager* Browser::create_content_manager()
{
    WebKitUserContentManager* manager = webkit_user_content_manager_new();

    // MPV bridge
    webkit_user_content_manager_register_script_message_handler(manager, "mpvPlayer");
    g_signal_connect(manager,
                     "script-message-received::mpvPlayer",
                     G_CALLBACK(Browser::s_mpv_message),
                     this);

    // xterm / Telehack bridge
    webkit_user_content_manager_register_script_message_handler(manager, "xtermLauncher");
    g_signal_connect(manager,
                     "script-message-received::xtermLauncher",
                     G_CALLBACK(Browser::s_xterm_message),
                     this);

    inject_user_script(manager);
    return manager;
}

void Browser::inject_user_script(WebKitUserContentManager* manager)
{
    if (script_source_.empty()) return;

    WebKitUserScript* script = webkit_user_script_new(
        script_source_.c_str(),
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        nullptr,
        nullptr
    );

    webkit_user_content_manager_add_script(manager, script);
    webkit_user_script_unref(script);
}

Browser::Tab& Browser::create_tab(const std::string& uri)
{
    Tab tab;

    tab.scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab.scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    WebKitUserContentManager* manager = create_content_manager();
    tab.webview = WEBKIT_WEB_VIEW(
        webkit_web_view_new_with_user_content_manager(manager));
    g_object_unref(manager);

    // Force black backing store to avoid white flashes between loads
    GdkRGBA black;
    black.red   = 0.0;
    black.green = 0.0;
    black.blue  = 0.0;
    black.alpha = 1.0;
    webkit_web_view_set_background_color(tab.webview, &black);

    gtk_container_add(GTK_CONTAINER(tab.scrolled),
                      GTK_WIDGET(tab.webview));

    // Tab label
    tab.label = gtk_label_new("New Tab");

    // Signals
    g_signal_connect(tab.webview, "load-changed",
                     G_CALLBACK(Browser::s_load_changed), this);
    g_signal_connect(tab.webview, "notify::uri",
                     G_CALLBACK(Browser::s_uri_changed), this);
    g_signal_connect(tab.webview, "notify::title",
                     G_CALLBACK(Browser::s_title_changed), this);

    // Add to notebook and vector
    gint page_num = gtk_notebook_append_page(GTK_NOTEBOOK(notebook_),
                                             tab.scrolled,
                                             tab.label);
    gtk_widget_show_all(tab.scrolled);

    tabs_.push_back(tab);
    current_tab_ = static_cast<int>(tabs_.size()) - 1;

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_), page_num);

    if (!uri.empty()) {
        webkit_web_view_load_uri(tab.webview, uri.c_str());
    }

    return tabs_.back();
}

WebKitWebView* Browser::current_webview()
{
    if (current_tab_ < 0 || current_tab_ >= static_cast<int>(tabs_.size())) {
        return nullptr;
    }
    return tabs_[current_tab_].webview;
}

Browser::Tab* Browser::get_tab_for_webview(WebKitWebView* view)
{
    for (auto& t : tabs_) {
        if (t.webview == view) return &t;
    }
    return nullptr;
}

// ───────────────────────────────────────────────
//  Navigation / loading
// ───────────────────────────────────────────────

void Browser::load_homepage()
{
    if (tabs_.empty()) {
        new_tab(COLOSSUS_HOMEPAGE);
    } else {
        load_uri(COLOSSUS_HOMEPAGE);
    }
}

void Browser::load_uri(const std::string& uri)
{
    WebKitWebView* view = current_webview();
    if (!view) {
        new_tab(uri);
        return;
    }
    webkit_web_view_load_uri(view, uri.c_str());
}

void Browser::update_url_entry_for(WebKitWebView* view)
{
    if (!view) return;

    const gchar* uri = webkit_web_view_get_uri(view);
    if (!uri) uri = "";

    if (view == current_webview() && url_entry_) {
        gtk_entry_set_text(GTK_ENTRY(url_entry_), uri);
    }
}

void Browser::update_tab_title_for(WebKitWebView* view)
{
    if (!view) return;

    const gchar* title = webkit_web_view_get_title(view);
    if (!title || !*title) {
        title = webkit_web_view_get_uri(view);
        if (!title) title = "Tab";
    }

    Tab* tab = get_tab_for_webview(view);
    if (tab && tab->label) {
        gtk_label_set_text(GTK_LABEL(tab->label), title);
    }
}

// ───────────────────────────────────────────────
//  Actions
// ───────────────────────────────────────────────

void Browser::new_tab(const std::string& uri)
{
    create_tab(uri);
}

void Browser::go_back()
{
    WebKitWebView* view = current_webview();
    if (view && webkit_web_view_can_go_back(view)) {
        webkit_web_view_go_back(view);
    }
}

void Browser::go_forward()
{
    WebKitWebView* view = current_webview();
    if (view && webkit_web_view_can_go_forward(view)) {
        webkit_web_view_go_forward(view);
    }
}

void Browser::go_home()
{
    load_homepage();
}

void Browser::reload()
{
    WebKitWebView* view = current_webview();
    if (view) {
        webkit_web_view_reload(view);
    }
}

// ───────────────────────────────────────────────
//  Public API
// ───────────────────────────────────────────────

void Browser::show()
{
    if (window_) {
        gtk_widget_show_all(window_);
    }
}

void Browser::open_uri(const std::string& uri)
{
    load_uri(uri);
}

// ───────────────────────────────────────────────
//  Event handlers (instance)
// ───────────────────────────────────────────────

void Browser::on_url_entry_activate()
{
    if (!url_entry_) return;

    const gchar* text = gtk_entry_get_text(GTK_ENTRY(url_entry_));
    if (!text) return;

    std::string input(text);
    if (input.empty()) return;

    // Very simple URL vs search detection
    std::string uri;
    if (input.find("://") != std::string::npos ||
        input.rfind("about:", 0) == 0 ||
        input.rfind("file:", 0) == 0) {
        uri = input;
    } else if (input.rfind("www.", 0) == 0) {
        uri = "https://" + input;
    } else if (input.find('.') != std::string::npos && input.find(' ') == std::string::npos) {
        uri = "https://" + input;
    } else {
        // Treat as search query (Brave)
        std::string encoded;
        for (char c : input) {
            if (c == ' ') encoded += '+';
            else encoded += c;
        }
        uri = "https://search.brave.com/search?q=" + encoded;
    }

    load_uri(uri);
}

void Browser::on_load_changed(WebKitWebView* view, WebKitLoadEvent event)
{
    if (event == WEBKIT_LOAD_FINISHED) {
        update_url_entry_for(view);
        update_tab_title_for(view);
    }
}

void Browser::on_uri_changed(WebKitWebView* view)
{
    update_url_entry_for(view);
}

void Browser::on_title_changed(WebKitWebView* view)
{
    update_tab_title_for(view);
}

void Browser::on_tab_switched(guint page_num)
{
    current_tab_ = static_cast<int>(page_num);
    update_url_entry_for(current_webview());
}

gboolean Browser::on_key_press(GdkEventKey* event)
{
    if (!event) return FALSE;

    // Ctrl+L: focus URL entry
    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_l) {
        if (url_entry_) {
            gtk_widget_grab_focus(url_entry_);
            gtk_editable_select_region(GTK_EDITABLE(url_entry_), 0, -1);
        }
        return TRUE;
    }

    // Ctrl+R or F5: reload
    if (((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_r) ||
        event->keyval == GDK_KEY_F5) {
        reload();
        return TRUE;
    }

    // Alt+Left / Alt+Right: back/forward
    if ((event->state & GDK_MOD1_MASK) && event->keyval == GDK_KEY_Left) {
        go_back();
        return TRUE;
    }
    if ((event->state & GDK_MOD1_MASK) && event->keyval == GDK_KEY_Right) {
        go_forward();
        return TRUE;
    }

    // Alt+H: home
    if ((event->state & GDK_MOD1_MASK) && event->keyval == GDK_KEY_h) {
        go_home();
        return TRUE;
    }

    // Alt+T: new tab
    if ((event->state & GDK_MOD1_MASK) && event->keyval == GDK_KEY_t) {
        new_tab(COLOSSUS_HOMEPAGE);
        return TRUE;
    }

    // Alt+V: send current page to mpv
    if ((event->state & GDK_MOD1_MASK) && event->keyval == GDK_KEY_v) {
        WebKitWebView* view = current_webview();
        if (view) {
            const gchar* uri = webkit_web_view_get_uri(view);
            if (uri && *uri) {
                launch_mpv(uri);
            }
        }
        return TRUE;
    }

    return FALSE;
}

// ───────────────────────────────────────────────
//  MPV / xterm bridges
// ───────────────────────────────────────────────

void Browser::launch_mpv(const std::string& url)
{
    std::string cmd;

    // Special-case Telehack Radio: audio-only, no ytdl-format, no filters
    if (url.find("telehack.com/radio") != std::string::npos ||
        url.find("telehack/radio") != std::string::npos) {
        cmd =
            "mpv --quiet "
            "--no-video "
            "--cache=yes --cache-secs=10 "
            "'" + url + "'";
    } else {
        // Default: 480p-preferred, software decode, amber/sepia filter for video
        cmd =
            "mpv --quiet "
            "--hwdec=no "
            "--vo=gpu "
            "--ytdl-format=bv*[height<=480]+ba/best[height<=480]/best "
            "--cache=yes --cache-secs=10 "
            "--vf=format=rgb24,hue=s=0,eq=brightness=-0.05:contrast=1.35:saturation=1.8 "
            "'" + url + "'";
    }

    GError* error = nullptr;
    if (!g_spawn_command_line_async(cmd.c_str(), &error)) {
        std::cerr << "Failed to launch mpv: "
                  << (error ? error->message : "unknown error")
                  << std::endl;
        if (error) g_error_free(error);
    }
}

void Browser::launch_xterm(const std::string& target)
{
    std::string cmd;

    if (target == "telehack") {
        cmd = "xterm -e telnet telehack.com";
    } else {
        cmd = "xterm";
    }

    GError* error = nullptr;
    if (!g_spawn_command_line_async(cmd.c_str(), &error)) {
        std::cerr << "Failed to launch xterm: "
                  << (error ? error->message : "unknown error")
                  << std::endl;
        if (error) g_error_free(error);
    }
}

void Browser::on_mpv_message(WebKitJavascriptResult* js_result)
{
    if (!js_result) return;

    JSCValue* value = webkit_javascript_result_get_js_value(js_result);
    if (!value || !jsc_value_is_string(value)) {
        return;
    }

    gchar* utf8 = jsc_value_to_string(value);
    if (!utf8) return;

    std::string url(utf8);
    g_free(utf8);

    if (!url.empty()) {
        launch_mpv(url);
    }
}

void Browser::on_xterm_message(WebKitJavascriptResult* js_result)
{
    if (!js_result) return;

    JSCValue* value = webkit_javascript_result_get_js_value(js_result);
    if (!value || !jsc_value_is_string(value)) {
        return;
    }

    gchar* utf8 = jsc_value_to_string(value);
    if (!utf8) return;

    std::string target(utf8);
    g_free(utf8);

    if (!target.empty()) {
        launch_xterm(target);
    }
}

// ───────────────────────────────────────────────
//  Static trampolines
// ───────────────────────────────────────────────

void Browser::s_load_changed(WebKitWebView* webview,
                             WebKitLoadEvent load_event,
                             gpointer user_data)
{
    auto* self = static_cast<Browser*>(user_data);
    if (!self) return;
    self->on_load_changed(webview, load_event);
}

void Browser::s_uri_changed(WebKitWebView* webview,
                            GParamSpec*,
                            gpointer user_data)
{
    auto* self = static_cast<Browser*>(user_data);
    if (!self) return;
    self->on_uri_changed(webview);
}

void Browser::s_title_changed(WebKitWebView* webview,
                              GParamSpec*,
                              gpointer user_data)
{
    auto* self = static_cast<Browser*>(user_data);
    if (!self) return;
    self->on_title_changed(webview);
}

void Browser::s_tab_switched(GtkNotebook*,
                             GtkWidget*,
                             guint page_num,
                             gpointer user_data)
{
    auto* self = static_cast<Browser*>(user_data);
    if (!self) return;
    self->on_tab_switched(page_num);
}

gboolean Browser::s_key_press(GtkWidget*,
                              GdkEventKey* event,
                              gpointer user_data)
{
    auto* self = static_cast<Browser*>(user_data);
    if (!self) return FALSE;
    return self->on_key_press(event);
}

void Browser::s_url_entry_activate(GtkEntry*,
                                   gpointer user_data)
{
    auto* self = static_cast<Browser*>(user_data);
    if (!self) return;
    self->on_url_entry_activate();
}

void Browser::s_mpv_message(WebKitUserContentManager*,
                            WebKitJavascriptResult* result,
                            gpointer user_data)
{
    auto* self = static_cast<Browser*>(user_data);
    if (!self) return;
    self->on_mpv_message(result);
}

void Browser::s_xterm_message(WebKitUserContentManager*,
                              WebKitJavascriptResult* result,
                              gpointer user_data)
{
    auto* self = static_cast<Browser*>(user_data);
    if (!self) return;
    self->on_xterm_message(result);
}

