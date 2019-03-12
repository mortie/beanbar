#ifndef BAR_H
#define BAR_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "ipc.h"

struct bar {
	int bar_height;
	enum { LOCATION_TOP, LOCATION_BOTTOM } location;
	char *rc;

	GtkWidget *win;
	WebKitWebView *webview;
	struct ipc ipc;
};

void bar_init(struct bar *bar, GtkApplication *app);
void bar_free(struct bar *bar);
void bar_show_debug(struct bar *bar);
void bar_trigger_update(struct bar *bar);

#endif
