#include "ipc.h"

#include <stdio.h>
#include <stdarg.h>

int ipc_loop() {
	while (1) {
		int c = getchar();
		if (c == '\n')
			return 1;
		else if (c == EOF)
			return 0;
	}
}

void ipc_send(const char *str) {
	printf("%s\n", str);
	fflush(stdout);
}

void ipc_sendf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	putchar('\n');
	fflush(stdout);
	va_end(ap);
}
