#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <glib-unix.h>

#include "bar.h"
#include "log.h"

int log_debug = 1;

struct opts {
	gint height;
	gboolean debug;
	gboolean debug_print;
	gboolean top;
	gboolean replace;
	gchar *config;
	gchar *monitor;
};

static struct opts opts = {
	.height = 26,
	.debug = FALSE,
	.top = FALSE,
	.config = NULL,
	.monitor = NULL,
};

static struct bar bar;

static void read_rc() {
	char *config = opts.config;
	char *defaultconf = NULL;

	if (opts.config == NULL) {
		char *conffile = "beanbar.js";
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

static void kill_existing() {
	pid_t selfpid = getpid();

	char selfpath[1024];
	ssize_t len;
	if ((len = readlink("/proc/self/exe", selfpath, sizeof(selfpath)) - 1) < 0) {
		perror("/proc/self/exe");
		return;
	}
	selfpath[len + 1] = '\0';

	DIR *dir = opendir("/proc");
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		int pid = atoi(ent->d_name);
		if (pid == 0) continue;
		if (pid == selfpid) continue;

		char exepath[64];
		snprintf(exepath, sizeof(exepath), "/proc/%i/exe", pid);

		char progpath[1024];
		if ((len = readlink(exepath, progpath, sizeof(progpath) - 1)) < 0) {
			if (errno != ENOENT && errno != EACCES) {
				perror(exepath);
			}
			continue;
		}
		progpath[len] = '\0';

		char *ignore = " (deleted)";
		if ((size_t)len > strlen(ignore)) {
			char *end = progpath + len - strlen(ignore);
			if (strcmp(end, ignore) == 0)
				progpath[len - strlen(ignore)] = '\0';
		}

		if (strcmp(selfpath, progpath) == 0) {
			debug("Waiting for beanbar %i to die...", pid);
			kill(pid, SIGTERM);
		}
	}
}

static void activate(GtkApplication *app, gpointer data) {
	log_debug = opts.debug;

	bar.bar_height = opts.height;
	bar.rc = "init(h(Label, { text: 'Missing config file.' }));";
	if (opts.top)
		bar.location = LOCATION_TOP;
	else
		bar.location = LOCATION_BOTTOM;
	bar.monitor = opts.monitor;
	bar.debug = opts.debug;

	if (opts.replace)
		kill_existing();

	read_rc();
	bar_init(&bar, app);
}

static int in_cleanup = 0;
static gboolean cleanup(gpointer data) {
	if (in_cleanup) {
		exit(EXIT_FAILURE);
	} else {
		in_cleanup = 1;
		debug("shutdown.");
		bar_free(&bar);
		exit(EXIT_FAILURE);
	}
	return TRUE;
}

static gboolean trigger_update(gpointer data) {
	bar_trigger_update(&bar);
	return TRUE;
}

static void shutdown(GtkApplication *app, gpointer data) {
	cleanup(0);
}

int main (int argc, char **argv) {
	GtkApplication *app;
	int status;

	signal(SIGCHLD, SIG_IGN);
	g_unix_signal_add(SIGTERM, cleanup, NULL);
	g_unix_signal_add(SIGINT, cleanup, NULL);
	g_unix_signal_add(SIGUSR2, trigger_update, NULL);

	GOptionEntry optents[] = {
		{
			"height", '\0', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &opts.height,
			"The height of the bar", NULL,
		}, {
			"debug", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts.debug,
			"Enable debug prints", NULL,
		}, {
			"top", 't', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts.top,
			"Place the bar on top", NULL,
		}, {
			"replace", 'r', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &opts.replace,
			"Replace a existing beanbar instances", NULL,
		}, {
			"config", 'c', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &opts.config,
			"The config file", NULL,
		}, {
			"monitor", 'm', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &opts.monitor,
			"The monitor to put the bar on", NULL,
		},
		{ 0 },
	};

	app = gtk_application_new("coffee.mort.beanbar", G_APPLICATION_NON_UNIQUE);
	g_application_add_main_option_entries(G_APPLICATION(app), optents);

	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	g_signal_connect(app, "shutdown", G_CALLBACK(shutdown), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
