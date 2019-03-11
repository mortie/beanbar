#include "providers.h"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "src/ipc.h"
#include "src/io.h"

static const char magic[] = { 'i', '3', '-', 'i', 'p', 'c' };

enum msgtype {
	I3_GET_WORKSPACES = 1,
	I3_SUBSCRIBE = 2,
};

static void i3_send(int fd, uint32_t t, char *pl, uint32_t pllen) {
	writeall(fd, magic, sizeof(magic));
	writeall(fd, &pllen, sizeof(pllen));
	writeall(fd, &t, sizeof(t));
	writeall(fd, pl, pllen);
}

static char *i3_recv(int fd, size_t *len, int *type) {
	static char *pl = NULL;
	static size_t pllen = 0;

	char m[sizeof(magic)];
	uint32_t l, t;
	readall(fd, m, sizeof(m));
	readall(fd, &l, sizeof(l));
	readall(fd, &t, sizeof(t));

	if (pllen < (l + 1)) {
		pllen = l + 1;
		pl = realloc(pl, pllen);
	}
	readall(fd, pl, l);
	pl[pllen - 1] = '\0';

	*len = l;
	*type = t;
	return pl;
}

static int main(int argc, char **argv) {
	(void)argc; (void)argv;

	FILE *f = popen("i3 --get-socketpath", "r");
	if (f == NULL) {
		perror("i3 --get-socketpath");
		return EXIT_FAILURE;
	}

	char sockpath[256];
	fscanf(f, "%255s\n", sockpath);
	fgets(sockpath, sizeof(sockpath) - 1, f);
	pclose(f);

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, sockpath, sizeof(addr.sun_path) - 1);
	addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror(sockpath);
		return EXIT_FAILURE;
	}

	{
		char str[] = "[\"workspace\",\"mode\"]";
		i3_send(sockfd, I3_SUBSCRIBE, str, strlen(str));
		size_t len;
		int t;
		char *pl = i3_recv(sockfd, &len, &t);
		if (strcmp(pl, "{\"success\":true}") != 0) {
			fprintf(stderr, "Failed to subscribe: %s\n", pl);
			return EXIT_FAILURE;
		}
	}

	i3_send(sockfd, I3_GET_WORKSPACES, NULL, 0);

	while (1) {
		size_t len;
		int t;
		char *msg = i3_recv(sockfd, &len, &t);
		ipc_send(msg);
	}

	return EXIT_SUCCESS;
}

struct prov prov_i3workspaces = { "i3workspaces", main };
