#include "bar.h"

struct opts {
	gboolean debug;
};

static struct opts opts = { 0 };
static struct bar bar;

static void activate(GtkApplication *app, gpointer data) {
	(void)data;

	bar.screen_width = 3840;
	bar.screen_height = 2160;
	bar.bar_height = 32;
	bar.location = LOCATION_BOTTOM;
	bar.rc =
		"init([\n"
		"    h(Label, { text: 'Hello Left' }),\n"
		"    [\n"
		"        h(Label, { text: 'Hello Right L' }),\n"
		"        h(Label, { text: 'Hello Right R' }),\n"
		"    ]\n"
		"]);";

	bar_create_window(&bar, app);

	if (opts.debug)
		bar_show_debug(&bar);
}

int main (int argc, char **argv) {
	GtkApplication *app;
	int status;

	GOptionEntry optents[] = {
		{
			"debug", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts.debug,
			"Show the webkit inspector", NULL,
		},
		{ 0 },
	};

	app = gtk_application_new("coffee.mort.webbar", G_APPLICATION_FLAGS_NONE);
	g_application_add_main_option_entries(G_APPLICATION(app), optents);

	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
