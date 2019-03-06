#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "bar.h"
#include "log.h"

int log_debug = 1;

struct opts {
	gint height;
	gboolean debug;
	gboolean debug_print;
	gboolean top;
	gchar *config;
};

static struct opts opts = {
	.height = 32,
	.debug = FALSE,
	.debug_print = FALSE,
	.top = FALSE,
	.config = NULL,
};

static struct bar bar;

static void read_rc() {
	char *config = opts.config;
	char *defaultconf = NULL;

	if (opts.config == NULL) {
		char *conffile = "webbar.js";
		const char *confdir = g_get_user_config_dir();
		size_t conflen = strlen(confdir) + 1 + strlen(conffile);
		defaultconf = g_malloc(conflen + 1);
		sprintf(defaultconf, "%s/%s", confdir, conffile);
		config = defaultconf;
	}

	struct stat st;
	if (stat(config, &st) < 0) {
		if (errno == ENOENT && opts.config == NULL) {
			warn("No --command argument and no %s.", config);
			g_free(defaultconf);
			return;
		}

		perror(config);
		exit(EXIT_FAILURE);
	}

	FILE *f = fopen(config, "r");
	if (f == NULL) {
		perror(opts.config);
		exit(EXIT_FAILURE);
	}

	bar.rc = g_malloc(st.st_size + 1);
	fread(bar.rc, 1, st.st_size, f);
	if (ferror(f)) {
		perror(config);
		exit(EXIT_FAILURE);
	}
	bar.rc[st.st_size] = '\0';

	g_free(defaultconf);
}

static void activate(GtkApplication *app, gpointer data) {
	(void)data;

	log_debug = opts.debug || opts.debug_print;

	bar.screen_width = 3840;
	bar.screen_height = 2160;
	bar.bar_height = opts.height;
	bar.rc = "init(h(Label, { text: 'Missing config file.' }));";
	if (opts.top)
		bar.location = LOCATION_TOP;
	else
		bar.location = LOCATION_BOTTOM;

	read_rc();
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
			"height", '\0', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &opts.height,
			"The height of the bar", NULL,
		}, {
			"debug", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts.debug,
			"Enable debugging, including inspector and debug prints", NULL,
		}, {
			"debug-print", 'p', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts.debug_print,
			"Enable debug prints", NULL,
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
