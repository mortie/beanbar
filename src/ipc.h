#ifndef IPC_H
#define IPC_H

#include <webkit2/webkit2.h>
#include <sys/epoll.h>
#include <pthread.h>

struct ipc_exec_ent {
	int active;
	int pid;
	int id;
	int infd;
	int outfd;
	int tmpfd;
	char tmpf[32];
};

struct ipc {
	WebKitWebView *view;
	int epollfd;
	pthread_t msgthread;
	pthread_mutex_t mutex;

	int msgpump_pipe[2];
	struct ipc_exec_ent *exec_ents;
	size_t exec_ents_len;
};

void ipc_init(struct ipc *ipc, WebKitWebView *view);
void ipc_free(struct ipc *ipc);
void ipc_trigger_update(struct ipc *ipc);

#endif
