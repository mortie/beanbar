#include "ipc.h"

#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "log.h"
#include "json.h"

static void send_msg_cb(GObject *source, GAsyncResult *res, gpointer data) {
	(void)source;
	(void)res;
	(void)data;
}

static void send_msg(struct ipc *ipc, int id, const char *msg) {
	const char fmt[] = "window.onIPCMessage(%10d, %s);";

	size_t msgjs_len;
	char *msgjs = json_stringify_string(msg, &msgjs_len);

	char *js = malloc(msgjs_len + 10 + sizeof(fmt));
	sprintf(js, fmt, id, msgjs);

	webkit_web_view_run_javascript(ipc->view, js, NULL, send_msg_cb, NULL);
	free(js);
}

struct idle_send_msg {
	struct ipc *ipc;
	int id;
	char msg[];
};

static gboolean idle_send_msg(void *data) {
	struct idle_send_msg *msg = (struct idle_send_msg *)data;
	send_msg(msg->ipc, msg->id, msg->msg);
	free(data);
	return FALSE;
}

static void handle_exec(struct ipc *ipc, int id, char *msg) {
	char *head = NULL;
	char *body = NULL;

	for (; *msg; msg += 1) {
		if (head == NULL) {
			if (!isspace(*msg)) {
				head = msg;
			}
		} else if (*msg == '\r' || *msg == '\n') {
			*msg = '\0';
			do msg += 1; while (*msg == '\r' || *msg == '\n');
			body = msg;
			break;
		}
	}

	if (head == NULL || body == NULL) {
		warn("Missing head or body (%p, %p).", head, body);
		return;
	}

	int infd[2];
	int outfd[2];
	if (pipe(infd) < 0 || pipe(outfd) < 0) {
		perror("pipe");
		return;
	}

	char tmpf[32];
	strcpy(tmpf, "/tmp/webbar.XXXXXX");
	int tmpfd = mkstemp(tmpf);
	if (tmpfd < 0) {
		perror("mkstemp");
		return;
	}
	if (write(tmpfd, body, strlen(body)) < 0) {
		perror("write");
		return;
	}

	debug("process %i: %s", id, head);
	debug("  TMPFILE=%s", tmpf);

	pid_t pid = fork();
	if (pid == 0) {
		close(infd[1]);
		close(outfd[0]);
		dup2(infd[0], STDIN_FILENO);
		dup2(outfd[1], STDOUT_FILENO);
		setenv("TMPFILE", tmpf, 1);
		char *args[] = { "sh", "-c", head, NULL };
		if (execvp(args[0], args) < 0) {
			perror(head);
			exit(EXIT_FAILURE);
		}
	} else {
		close(infd[0]);
		close(outfd[1]);

		// Find available ent, or alloc space for it if necessary
		size_t idx;
		for (idx = 0; idx < ipc->exec_ents_len; ++idx)
			if (!ipc->exec_ents[idx].active) break;
		if (idx == ipc->exec_ents_len) {
			ipc->exec_ents = realloc(
				ipc->exec_ents, ++ipc->exec_ents_len * sizeof(*ipc->exec_ents));
		}

		// Create entry
		struct ipc_exec_ent *ent = ipc->exec_ents + idx;
		memset(ent, 0, sizeof(*ent));
		ent->active = 1;
		ent->pid = pid;
		ent->id = id;
		ent->infd = infd[1];
		ent->outfd = outfd[0];
		ent->tmpfd = tmpfd;
		strcpy(ent->tmpf, tmpf);
		struct epoll_event ev = { 0 };
		ev.events = EPOLLIN;
		ev.data.fd = ent->outfd;
		if (epoll_ctl(ipc->epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev) < 0) {
			perror("epoll");
			return;
		}
	}
}

static void on_msg(
		WebKitUserContentManager *manager, WebKitJavascriptResult *jsresult,
		gpointer data) {
	(void)manager;
	struct ipc *ipc = (struct ipc *)data;
	JSCValue *val = webkit_javascript_result_get_js_value(jsresult);

	if (!jsc_value_is_object(val)) {
		warn("IPC expected object.");
		return;
	}

	JSCValue *valid = jsc_value_object_get_property(val, "id");
	if (valid == NULL) {
		warn("IPC expected object with an `id` property.");
		return;
	}

	JSCValue *valmsg = jsc_value_object_get_property(val, "msg");
	if (valmsg == NULL) {
		warn("IPC expected object with a `msg` property.");
		return;
	}

	JSCValue *valtype = jsc_value_object_get_property(val, "type");
	if (valtype == NULL) {
		warn("IPC expected object with a `type` property.");
		return;
	}

	char *type = jsc_value_to_string(valtype);

	if (strcmp(type, "exec") == 0) {
		handle_exec(ipc, (int)jsc_value_to_double(valid), jsc_value_to_string(valmsg));
	} else {
		warn("Unknown IPC message type: %s", type);
	}
}

static void *message_pump(void *data) {
	struct ipc *ipc = (struct ipc *)data;
	const int count = 10;
	struct epoll_event events[count];

	while (1) {
		int ret = epoll_wait(ipc->epollfd, events, count, -1);
		if (ret < 0) {
			perror("epoll_wait");
			continue;
		}

		for (int i = 0; i < ret; ++i) {
			struct epoll_event *ev = events + i;
			if (ev->data.fd == ipc->msgpump_pipe[0])
				return NULL;

			size_t idx;
			for (idx = 0; idx < ipc->exec_ents_len; ++idx)
				if (ipc->exec_ents[idx].outfd == ev->data.fd)
					break;
			if (idx == ipc->exec_ents_len) {
				warn("got data on bad FD: %i\n", ev->data.fd);
				continue;
			}
			struct ipc_exec_ent *ent = ipc->exec_ents + idx;

			char buf[1024];
			ssize_t num = read(ev->data.fd, buf, sizeof(buf) - 1);
			buf[num] = '\0';

			if (num < 0) {
				perror("read");
				continue;
			} else if (num == 0) {
				int status;
				waitpid(ent->pid, &status, 0);
				debug("process %i died (code: %i)", ent->id, WEXITSTATUS(status));
				ent->active = 0;
				close(ent->tmpfd);
				unlink(ent->tmpf);
				if (epoll_ctl(ipc->epollfd, EPOLL_CTL_DEL, ev->data.fd, NULL) < 0)
					perror("epoll_ctl");
				continue;
			}

			// Send the message some time on the main thread
			struct idle_send_msg *msg = malloc(sizeof(struct idle_send_msg) + num + 1);
			msg->ipc = ipc;
			msg->id = ent->id;
			strcpy(msg->msg, buf);
			g_idle_add(idle_send_msg, msg);
		}
	}

	return NULL;
}

void ipc_init(struct ipc *ipc, WebKitWebView *view) {
	ipc->view = view;
	ipc->epollfd = epoll_create1(0);
	ipc->exec_ents = NULL;
	ipc->exec_ents_len = 0;
	WebKitUserContentManager *webmgr = webkit_web_view_get_user_content_manager(view);
	g_signal_connect(webmgr, "script-message-received::ipc",
		G_CALLBACK(on_msg), ipc);
	webkit_user_content_manager_register_script_message_handler(webmgr, "ipc");

	if (pipe(ipc->msgpump_pipe) < 0) {
		perror("pipe");
		return;
	}

	struct epoll_event ev = { 0 };
	ev.events = EPOLLIN;
	ev.data.fd = ipc->msgpump_pipe[0];
	epoll_ctl(ipc->epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev);

	if (pthread_create(&ipc->msgthread, NULL, message_pump, ipc) < 0) {
		perror("pthread");
	}
}

void ipc_free(struct ipc *ipc) {
	char buf[] = "";
	write(ipc->msgpump_pipe[1], buf, sizeof(buf));
	void *retval;
	pthread_join(ipc->msgthread, &retval);

	for (size_t i = 0; i < ipc->exec_ents_len; ++i) {
		struct ipc_exec_ent *ent = ipc->exec_ents + i;
		if (!ent->active) continue;
		debug("waiting for process %i (%i)...", ent->id, ent->pid);
		kill(ent->pid, SIGTERM);
		waitpid(ent->pid, NULL, 0);
		close(ent->tmpfd);
		unlink(ent->tmpf);
	}
	free(ipc->exec_ents);
}