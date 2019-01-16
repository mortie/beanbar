#include <stdio.h>
#include <stdint.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

extern const uint8_t _binary_web_html_start[];
extern const uint8_t _binary_web_html_end;

struct config {
	int screen_width;
	int screen_height;
	int bar_height;
	double scaling;
	enum { LOCATION_TOP, LOCATION_BOTTOM } location;
};

static void set_prop_atom(GdkWindow *win, const char *key, const char *val) {
	GdkAtom atom = gdk_atom_intern_static_string(val);
	gdk_property_change(win,
		gdk_atom_intern_static_string(key),
		gdk_atom_intern_static_string("ATOM"),
		32, GDK_PROP_MODE_REPLACE,
		(guchar *)&atom, 1);
}

static void set_prop_cardinal(GdkWindow *win, const char *key, const void *val, int count) {
	gdk_property_change(win,
		gdk_atom_intern_static_string(key),
		gdk_atom_intern_static_string("CARDINAL"),
		32, GDK_PROP_MODE_REPLACE,
		(guchar *)val, count);
}

static void uri_handler_builtin(WebKitURISchemeRequest *req, gpointer data) {
	(void)data;
	size_t size = (size_t)(&_binary_web_html_end - _binary_web_html_start);
	GInputStream *stream = g_memory_input_stream_new_from_data(
		_binary_web_html_start, size, NULL);
	webkit_uri_scheme_request_finish(req, stream, size, "text/html");
}

static void activate(GtkApplication* app, gpointer data) {
	struct config *conf = (struct config *)data;

	// Create window
	GtkWidget *window = gtk_application_window_new (app);
	gtk_window_set_title(GTK_WINDOW(window), "WebBar");
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(window), conf->screen_width, conf->bar_height);

	// Add WebKitGTK
	WebKitWebView *web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
	WebKitWebContext *web_ctx = webkit_web_view_get_context(web_view);
	webkit_web_context_register_uri_scheme(
		web_ctx, "builtin", uri_handler_builtin, NULL, NULL);
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(web_view));
	webkit_web_view_load_uri(web_view, "builtin:");
	webkit_web_view_set_zoom_level(web_view, conf->scaling);
	gtk_widget_grab_focus(GTK_WIDGET(web_view));

	// Show
	gtk_widget_show_all(window);
	GdkWindow *gdkwin = gtk_widget_get_window(window);

	// Make the window a dock
	set_prop_atom(gdkwin, "_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_DOCK");

	// Set dock location
	long strut_partial[12];
	switch (conf->location) {
	case LOCATION_TOP:
		memcpy(strut_partial, (long[]) {
			0, 0, conf->bar_height, 0,
			0, 0, 0, 0,
			0, conf->screen_width - 1, 0, 0,
		}, sizeof(strut_partial));
		break;
	case LOCATION_BOTTOM:
		memcpy(strut_partial, (long[]) {
			0, 0, 0, conf->bar_height,
			0, 0, 0, 0,
			0, 0, 0, conf->screen_width - 1,
		}, sizeof(strut_partial));
		break;
	}
	set_prop_cardinal(gdkwin, "_NET_WM_STRUT_PARTIAL", (const void *)strut_partial, 12);
}

int main (int argc, char **argv) {
	GtkApplication *app;
	int status;

	struct config conf = { 3840, 2160, 64, 2.0, LOCATION_BOTTOM };

	app = gtk_application_new("coffee.mort.webbar", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), (gpointer)&conf);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
