#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "bar.h"
#include "log.h"

int log_debug = 1;

struct opts {
	gboolean debug;
	gboolean debug_print;
	gint height;
	gboolean top;
	gchar *config;
};

static struct opts opts = {
	.debug = FALSE,
	.debug_print = FALSE,
	.height = 32,
	.top = FALSE,
	.config = NULL,
};

static struct bar bar;

static void activate(GtkApplication *app, gpointer data) {
	(void)data;

	log_debug = opts.debug || opts.debug_print;

	bar.screen_width = 3840;
	bar.screen_height = 2160;
	bar.bar_height = opts.height;
	bar.rc = "init(h(Label, { text: 'Missing --config argument.' }));";
	if (opts.top)
		bar.location = LOCATION_TOP;
	else
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

		bar.rc = malloc(st.st_size + 1);
		fread(bar.rc, 1, st.st_size, f);
		if (ferror(f)) {
			perror(opts.config);
			exit(EXIT_FAILURE);
		}
		bar.rc[st.st_size] = '\0';
	} else {
		warn("Missing config argument.");
	}

	bar_init(&bar, app);

	if (opts.debug)
		bar_show_debug(&bar);
}

static int in_cleanup = 0;
static void cleanup(int sig) {
	(void)sig;
	if (in_cleanup) {
		exit(EXIT_FAILURE);
	} else {
		in_cleanup = 1;
		debug("shutdown.");
		bar_free(&bar);
		exit(EXIT_FAILURE);
	}
}

static void shutdown(GtkApplication *app, gpointer data) {
	(void)app;
	(void)data;
	cleanup(0);
}

int main (int argc, char **argv) {
	GtkApplication *app;
	int status;

	signal(SIGCHLD, SIG_IGN);
	signal(SIGTERM, cleanup);
	signal(SIGINT, cleanup);

	GOptionEntry optents[] = {
		{
			"debug", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts.debug,
			"Enable debugging, including inspector and debug prints", NULL,
		}, {
			"debug-print", 'p', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts.debug_print,
			"Enable debug prints", NULL,
		}, {
			"height", '\0', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &opts.height,
			"The height of the bar", NULL,
		}, {
			"top", 't', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts.top,
			"Place the bar on top", NULL,
		}, {
			"config", 'c', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &opts.config,
			"The config file", NULL,
		},
		{ 0 },
	};

	app = gtk_application_new("coffee.mort.webbar", G_APPLICATION_FLAGS_NONE);
	g_application_add_main_option_entries(G_APPLICATION(app), optents);

	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	g_signal_connect(app, "shutdown", G_CALLBACK(shutdown), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
