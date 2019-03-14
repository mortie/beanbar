#include "bar.h"

#include <stdint.h>

#include "log.h"

extern const uint8_t _binary_obj_web_html_start[];
extern const uint8_t _binary_obj_web_html_end;

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
	size_t size = (size_t)(&_binary_obj_web_html_end - _binary_obj_web_html_start);
	GInputStream *stream = g_memory_input_stream_new_from_data(
		_binary_obj_web_html_start, size, NULL);
	webkit_uri_scheme_request_finish(req, stream, size, "text/html");
	g_object_unref(stream);
}

static void uri_handler_config(WebKitURISchemeRequest *req, gpointer data) {
	struct bar *bar = (struct bar *)data;
	size_t size = strlen(bar->rc);
	GInputStream *stream = g_memory_input_stream_new_from_data(
		bar->rc, size, NULL);
	webkit_uri_scheme_request_finish(req, stream, size, "text/html");
	g_object_unref(stream);
}

void bar_init(struct bar *bar, GtkApplication *app) {
	GdkDisplay *disp = gdk_display_get_default();
	GdkMonitor *mon = NULL;
	if (bar->monitor == NULL) {
		mon = gdk_display_get_primary_monitor(disp);
	} else {
		int nmons = gdk_display_get_n_monitors(disp);
		for (int i = 0; i < nmons; ++i) {
			GdkMonitor *m = gdk_display_get_monitor(disp, i);
			const char *model = gdk_monitor_get_model(m);
			if (strcmp(bar->monitor, model) == 0) {
				mon = m;
				break;
			}
		}
	}

	if (mon == NULL) {
		err("Monitor not found.");
		exit(EXIT_FAILURE);
	}

	GdkRectangle geometry;
	gdk_monitor_get_geometry(mon, &geometry);
	int scale = gdk_monitor_get_scale_factor(mon);

	bar->win = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(bar->win), "Beanbar");
	gtk_window_set_decorated(GTK_WINDOW(bar->win), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(bar->win), geometry.width, bar->bar_height);
	gtk_window_move(GTK_WINDOW(bar->win), geometry.x, geometry.y + geometry.height - bar->bar_height);

	// Create WebKitGTK
	bar->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());

	WebKitSettings *settings = webkit_web_view_get_settings(bar->webview);
	webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);

	WebKitWebContext *webctx = webkit_web_view_get_context(bar->webview);
	webkit_web_context_register_uri_scheme(
		webctx, "builtin", uri_handler_builtin, bar, NULL);
	webkit_web_context_register_uri_scheme(
		webctx, "config", uri_handler_config, bar, NULL);

	// Create IPC thing
	ipc_init(&bar->ipc, bar->webview);

	// Show
	webkit_web_view_load_uri(bar->webview, "builtin:");
	gtk_container_add(GTK_CONTAINER(bar->win), GTK_WIDGET(bar->webview));
	gtk_widget_grab_focus(GTK_WIDGET(bar->webview));
	gtk_widget_show_all(bar->win);

	GdkWindow *gdkwin = gtk_widget_get_window(bar->win);

	// Make the bar->win a dock
	set_prop_atom(gdkwin, "_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_DOCK");
	set_prop_atom(gdkwin, "_NET_WM_WINDOW_STATE", "_NET_WM_STATE_ABOVE");

	// Set dock location
	long strut_partial[12];
	switch (bar->location) {
	case LOCATION_TOP:
		memcpy(strut_partial, (long[]) {
			0, 0, bar->bar_height * scale, 0,
			0, 0, 0, 0,
			geometry.x * scale, (geometry.x + geometry.width) * scale - 1, 0, 0,
		}, sizeof(strut_partial));
		break;
	case LOCATION_BOTTOM:
		memcpy(strut_partial, (long[]) {
			0, 0, 0, bar->bar_height * scale,
			0, 0, 0, 0,
			0, 0, geometry.x * scale, (geometry.x + geometry.width) * scale - 1,
		}, sizeof(strut_partial));
		break;
	}

	set_prop_cardinal(gdkwin, "_NET_WM_STRUT", (const void *)strut_partial, 4);
	set_prop_cardinal(gdkwin, "_NET_WM_STRUT_PARTIAL", (const void *)strut_partial, 12);

	long opaque_region[] = {
		geometry.x * scale, (geometry.y + geometry.height - bar->bar_height) * scale,
		geometry.width * scale, bar->bar_height * scale,
	};
	set_prop_cardinal(gdkwin, "_NET_WM_OPAQUE_REGION", (const void *)opaque_region, 4);
}

void bar_free(struct bar *bar) {
	ipc_free(&bar->ipc);
}

void bar_show_debug(struct bar *bar) {
	WebKitSettings *web_settings = webkit_web_view_get_settings(bar->webview);
	g_object_set(G_OBJECT(web_settings), "enable-developer-extras", TRUE, NULL);
	WebKitWebInspector *inspector = webkit_web_view_get_inspector(bar->webview);
	webkit_web_inspector_show(inspector);
}

void bar_trigger_update(struct bar *bar) {
	ipc_trigger_update(&bar->ipc);
}
