// main.cpp — entry point for COLOSSUS Browser

#include <gtk/gtk.h>
#include "browser.h"

// Global browser instance (same pattern you had before)
static Browser* g_browser = nullptr;

// ───────────────────────────────────────────────
//  Load app-local GTK CSS theme
// ───────────────────────────────────────────────
static void load_colossus_gtk_theme()
{
    GtkCssProvider* provider = gtk_css_provider_new();
    GError* error = nullptr;

    const char* css_path = "resources/gtk-colossus.css";

    gtk_css_provider_load_from_path(provider, css_path, &error);

    if (error) {
        g_printerr("Failed to load Colossus GTK theme (%s): %s\n",
                   css_path, error->message);
        g_error_free(error);
    } else {
        GdkScreen* screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(
            screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION  // app-level override
        );
    }

    g_object_unref(provider);
}

// Called once at startup, before any windows/widgets exist
static void on_app_startup(GtkApplication* app, gpointer user_data)
{
    (void)app;
    (void)user_data;
    load_colossus_gtk_theme();
}

// Called whenever the app is activated (first launch or re-activation)
static void on_app_activate(GtkApplication* app, gpointer user_data)
{
    (void)user_data;

    if (!g_browser) {
        g_browser = new Browser(app);
    }

    g_browser->show();
}

int main(int argc, char** argv)
{
    GtkApplication* app =
        gtk_application_new("tech.will.colossus", G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "startup",
                     G_CALLBACK(on_app_startup), nullptr);
    g_signal_connect(app, "activate",
                     G_CALLBACK(on_app_activate), nullptr);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    // Cleanup
    delete g_browser;
    g_browser = nullptr;

    g_object_unref(app);
    return status;
}

