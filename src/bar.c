#include "bar.h"

#include <stdint.h>

#include "log.h"
#include "dispserv.h"

extern const uint8_t _binary_obj_web_html_start[];
extern const uint8_t _binary_obj_web_html_end;

static GInputStream *serve_file(const char *path, const char **mime) {
	char *type = g_content_type_guess(path, NULL, 0, NULL);
	char *m = g_content_type_get_mime_type(type);
	free(type);

	if (m != NULL) {
		*mime = m;
	}

	GFile *f = g_file_new_for_path(path);
	GError *err = NULL;
	GInputStream *stream = G_INPUT_STREAM(g_file_read(f, NULL, &err));
	g_object_unref(f);
	if (err != NULL) {
		warn("Reading %s failed: %s", path, err->message);
		return NULL;
	}

	return stream;
}

static void uri_handler_local(WebKitURISchemeRequest *req, gpointer data) {
	struct bar *bar = (struct bar *)data;
	const char *url = webkit_uri_scheme_request_get_path(req);

	GInputStream *stream;
	size_t size = -1;
	const char *mime = "application/octet-stream";

	if (strcmp(url, "/") == 0) {
		mime = "text/html";
		size = (size_t)(&_binary_obj_web_html_end - _binary_obj_web_html_start);
		stream = g_memory_input_stream_new_from_data(
			_binary_obj_web_html_start, size, NULL);
	} else if (strcmp(url, "conf") == 0) {
		mime = "application/javascript";
		if (bar->rcfile == NULL) {
			const char str[] = "init(h(Label, { text: 'Missing config file.' }));";
			size = sizeof(str) - 1;
			stream = g_memory_input_stream_new_from_data(str, sizeof(str), NULL);
		} else {
			stream = serve_file(bar->rcfile, &mime);
		}
	} else {
		if (bar->rcdir == NULL) {
			warn("Got a request for a file (%s) without a config dir.", url);
			return;
		}

		char *path = g_build_path("/", bar->rcdir, url, NULL);
		stream = serve_file(path, &mime);
	}

	if (stream == NULL)
		return;

	debug("Request: local:%s (%s)", url, mime);
	webkit_uri_scheme_request_finish(req, stream, size, mime);
	g_object_unref(stream);
}

static void create_win(struct bar *bar, struct bar_win *win, GdkMonitor *mon) {
	g_object_ref(mon);
	win->mon = mon;

	win->win = gtk_application_window_new(bar->app);
	gtk_window_set_title(GTK_WINDOW(win->win), "Beanbar");
	gtk_window_set_decorated(GTK_WINDOW(win->win), FALSE);

	// Create WebKitGTK
	win->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
	WebKitSettings *settings = webkit_web_view_get_settings(win->webview);
	WebKitWebContext *webctx = webkit_web_view_get_context(win->webview);
	WebKitSecurityManager *secmgr = webkit_web_context_get_security_manager(webctx);

	// Configure WebKitGTK
	webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);
	g_object_set(G_OBJECT(settings), "enable-developer-extras", TRUE, NULL);
	webkit_web_context_register_uri_scheme(
		webctx, "local", uri_handler_local, bar, NULL);
	webkit_security_manager_register_uri_scheme_as_secure(secmgr, "local");
	webkit_security_manager_register_uri_scheme_as_cors_enabled(secmgr, "local");

	// Create IPC thing
	ipc_init(&win->ipc, win->webview);

	// Show WebKitGTK
	webkit_web_view_load_uri(win->webview, "local:/");
	gtk_container_add(GTK_CONTAINER(win->win), GTK_WIDGET(win->webview));
	gtk_widget_grab_focus(GTK_WIDGET(win->webview));
	gtk_widget_show_all(GTK_WIDGET(win->webview));

	dispserv_init_win(bar, win);
}

static void destroy_win(struct bar *bar, struct bar_win *win) {
	gtk_widget_destroy(win->win);
	g_object_unref(win->webview);
	g_object_unref(win->win);
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
	if (strcmp(bar->monitor, "primary") == 0) {
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
	free(bar->wins);
}

void bar_trigger_update(struct bar *bar) {
	for (size_t i = 0; i < bar->wins_len; ++i) {
		ipc_trigger_update(&bar->wins[i].ipc);
	}
}
