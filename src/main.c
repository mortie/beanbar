#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bar.h"
#include "log.h"

struct opts {
	gboolean debug;
	gint height;
	gchar *config;
};

static struct opts opts = {
	.debug = FALSE,
	.height = 32,
	.config = NULL,
};
static struct bar bar;

static void activate(GtkApplication *app, gpointer data) {
	(void)data;

	bar.screen_width = 3840;
	bar.screen_height = 2160;
	bar.bar_height = 32;
	bar.rc = "init(h(Label, { text: 'Missing --config argument.' }));";
	bar.location = LOCATION_BOTTOM;

	if (opts.config != NULL) {
		struct stat st;
		if (stat(opts.config, &st) < 0) {
			perror(opts.config);
			exit(EXIT_FAILURE);
		}

		FILE *f = fopen(opts.config, "r");
		if (f == NULL) {
			perror(opts.config);
			exit(EXIT_FAILURE);
		}

		bar.rc = malloc(st.st_size);
		fread(bar.rc, 1, st.st_size, f);
		if (ferror(f)) {
			perror(opts.config);
			exit(EXIT_FAILURE);
		}
	} else {
		warn("Missing config argument.");
	}

	bar_create_window(&bar, app);

	if (opts.debug)
		bar_show_debug(&bar);
}

int main (int argc, char **argv) {
	GtkApplication *app;
	int status;

	signal(SIGCHLD, SIG_IGN);

	GOptionEntry optents[] = {
		{
			"debug", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts.debug,
			"Show the webkit inspector", NULL,
		}, {
			"height", '\0', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &opts.height,
			"The height of the bar", NULL,
		}, {
			"config", 'c', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &opts.config,
			"The config file", NULL,
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
