#ifndef IPC_H
#define IPC_H

#include <webkit2/webkit2.h>

struct ipc {
	WebKitWebView *view;
};

void ipc_init(struct ipc *ipc, WebKitWebView *view);

#endif
