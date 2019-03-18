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

static void create_win(struct bar *bar, struct bar_win *win, GdkMonitor *mon) {
	g_object_ref(mon);
	win->mon = mon;

	GdkRectangle geometry;
	gdk_monitor_get_geometry(mon, &geometry);
	int scale = gdk_monitor_get_scale_factor(mon);

	win->win = gtk_application_window_new(bar->app);
	gtk_window_set_title(GTK_WINDOW(win->win), "Beanbar");
	gtk_window_set_decorated(GTK_WINDOW(win->win), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(win->win), geometry.width, bar->bar_height);

	// Create WebKitGTK
	win->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());

	WebKitSettings *settings = webkit_web_view_get_settings(win->webview);
	webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);
	g_object_set(G_OBJECT(settings), "enable-developer-extras", TRUE, NULL);

	WebKitWebContext *webctx = webkit_web_view_get_context(win->webview);
	webkit_web_context_register_uri_scheme(
		webctx, "builtin", uri_handler_builtin, bar, NULL);
	webkit_web_context_register_uri_scheme(
		webctx, "config", uri_handler_config, bar, NULL);

	// Create IPC thing
	ipc_init(&win->ipc, win->webview);

	// Show
	webkit_web_view_load_uri(win->webview, "builtin:");
	gtk_container_add(GTK_CONTAINER(win->win), GTK_WIDGET(win->webview));
	gtk_widget_grab_focus(GTK_WIDGET(win->webview));
	gtk_widget_show_all(win->win);

	GdkWindow *gdkwin = gtk_widget_get_window(win->win);

	// Make the window a dock
	set_prop_atom(gdkwin, "_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_DOCK");
	set_prop_atom(gdkwin, "_NET_WM_WINDOW_STATE", "_NET_WM_STATE_ABOVE");

	// Move/resize window
	gtk_window_move(GTK_WINDOW(win->win), geometry.x, geometry.y + geometry.height - bar->bar_height);
	gtk_window_resize (GTK_WINDOW(win->win), geometry.width, bar->bar_height);

	// Set struts
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

	// Set the opaque region
	long opaque_region[] = {
		geometry.x * scale, (geometry.y + geometry.height - bar->bar_height) * scale,
		geometry.width * scale, bar->bar_height * scale,
	};
	set_prop_cardinal(gdkwin, "_NET_WM_OPAQUE_REGION", (const void *)opaque_region, 4);
}

static void destroy_win(struct bar *bar, struct bar_win *win) {
	gtk_widget_destroy(win->win);
	g_object_unref(win->mon);
	ipc_free(&win->ipc);
}

void on_monitor_added(GdkDisplay *display, GdkMonitor *mon, gpointer data) {
	struct bar *bar = (struct bar *)data;
	debug("On monitor added %s (%p)", gdk_monitor_get_model(mon), (void *)mon);

	if (bar->wins_len >= bar->wins_size) {
		bar->wins_size *= 2;
		bar->wins = realloc(bar->wins, bar->wins_size * sizeof(*bar->wins));
	}

	create_win(bar, &bar->wins[bar->wins_len], mon);
	bar->wins_len += 1;
}

void on_monitor_removed(GdkDisplay *display, GdkMonitor *mon, gpointer data) {
	struct bar *bar = (struct bar *)data;
	debug("On monitor removed %s (%p)", gdk_monitor_get_model(mon), (void *)mon);

	for (size_t i = 0; i < bar->wins_len; ++i) {
		if (bar->wins[i].mon != mon) continue;

		destroy_win(bar, &bar->wins[i]);
		bar->wins_len -= 1;
		memmove(&bar->wins[i], &bar->wins[i + 1], bar->wins_len - i);
		break;
	}
}

static void init_static(struct bar *bar) {
	GdkDisplay *disp = gdk_display_get_default();
	GdkMonitor *mon = NULL;
	if (strcmp(bar->monitor, "primary")) {
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

	bar->wins_size = 4;
	bar->wins_len = 1;
	bar->wins = malloc(bar->wins_size * sizeof(*bar->wins));
	create_win(bar, &bar->wins[0], mon);
}

static void init_dynamic(struct bar *bar) {
	GdkDisplay *disp = gdk_display_get_default();
	int nmons = gdk_display_get_n_monitors(disp);

	bar->wins_size = nmons < 4 ? 4 : nmons;
	bar->wins_len = nmons;
	bar->wins = malloc(bar->wins_size * sizeof(*bar->wins));

	for (int i = 0; i < nmons; ++i) {
		GdkMonitor *m = gdk_display_get_monitor(disp, i);
		create_win(bar, &bar->wins[i], m);
	}

	g_signal_connect(disp, "monitor-added", G_CALLBACK(on_monitor_added), bar);
	g_signal_connect(disp, "monitor-removed", G_CALLBACK(on_monitor_removed), bar);
}

void bar_init(struct bar *bar, GtkApplication *app) {
	bar->app = app;

	if (bar->monitor == NULL)
		init_dynamic(bar);
	else
		init_static(bar);
}

void bar_free(struct bar *bar) {
	for (size_t i = 0; i < bar->wins_len; ++i) {
		ipc_free(&bar->wins[i].ipc);
	}
}

void bar_trigger_update(struct bar *bar) {
	for (size_t i = 0; i < bar->wins_len; ++i) {
		ipc_trigger_update(&bar->wins[i].ipc);
	}
}
