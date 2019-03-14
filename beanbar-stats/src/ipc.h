#ifndef IPC_H
#define IPC_H

int ipc_loop();
void ipc_send(const char *str);
void ipc_sendf(const char *fmt, ...);

#endif
