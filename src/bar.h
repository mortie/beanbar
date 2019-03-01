#ifndef BAR_H
#define BAR_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "ipc.h"

struct bar {
	int screen_width;
	int screen_height;
	int bar_height;
	enum { LOCATION_TOP, LOCATION_BOTTOM } location;
	char *rc;

	GtkWidget *win;
	WebKitWebView *webview;
	struct ipc ipc;
};

struct bar *bar_create();
void bar_create_window(struct bar *bar, GtkApplication *app);
void bar_show_debug(struct bar *bar);

#endif
