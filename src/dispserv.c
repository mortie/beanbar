#include "dispserv.h"

#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#endif
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "log.h"

#ifdef GDK_WINDOWING_WAYLAND
static void init_wayland(struct bar *bar, struct bar_win *win) {
	GdkRectangle geometry;
	gdk_monitor_get_geometry(win->mon, &geometry);
	gtk_window_set_default_size(GTK_WINDOW(win->win), geometry.width, bar->bar_height);

	gtk_layer_init_for_window(GTK_WINDOW(win->win));
	gtk_layer_set_layer(GTK_WINDOW(win->win), GTK_LAYER_SHELL_LAYER_TOP);
	gtk_layer_auto_exclusive_zone_enable(GTK_WINDOW(win->win));
	gtk_layer_set_monitor(GTK_WINDOW(win->win), win->mon);

	switch (bar->location) {
	case LOCATION_TOP:
		gtk_layer_set_anchor(GTK_WINDOW(win->win), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
		break;
	case LOCATION_BOTTOM:
		gtk_layer_set_anchor(GTK_WINDOW(win->win), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
		break;
	}

	gtk_layer_set_anchor(GTK_WINDOW(win->win), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
	gtk_layer_set_anchor(GTK_WINDOW(win->win), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

	gtk_widget_show_all(win->win);
}
#endif

#ifdef GDK_WINDOWING_X11
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

static void init_x11(struct bar *bar, struct bar_win *win) {
	GdkRectangle geometry;
	gdk_monitor_get_geometry(win->mon, &geometry);
	int scale = gdk_monitor_get_scale_factor(win->mon);
	gtk_window_set_default_size(GTK_WINDOW(win->win), geometry.width, bar->bar_height);

	// We want to change gdk stuff _before_ starting webkitgtk,
	// because window managers seem to get confused if it takes too long time
	// from we create the window to we set the props
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
#endif

void dispserv_init_win(struct bar *bar, struct bar_win *win) {
#ifdef GDK_WINDOWING_WAYLAND
	if (GDK_IS_WAYLAND_DISPLAY(gdk_monitor_get_display(win->mon))) {
		init_wayland(bar, win);
		return;
	}
#endif

#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(gdk_monitor_get_display(win->mon))) {
		init_x11(bar, win);
		return;
	}
#endif

	err("Unsupported GDK backend.");
	abort();
}
