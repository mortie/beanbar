#ifndef IPC_H
#define IPC_H

#include <webkit2/webkit2.h>
#include <sys/epoll.h>
#include <pthread.h>

struct ipc_ent_exec {
	int active;
	struct epoll_event ev;
	int id;
};

struct ipc {
	WebKitWebView *view;
	int epollfd;
	pthread_t msgthread;

	struct ipc_ent_exec *exec_ents;
	size_t exec_ents_len;
};

void ipc_init(struct ipc *ipc, WebKitWebView *view);

#endif
