#include "io.h"

#include <unistd.h>
#include <stdio.h>

void writeall(int fd, const void *buf, size_t buflen) {
	const char *b = buf;
	while (buflen > 0) {
		ssize_t n = write(fd, b, buflen);
		if (n < 0) {
			perror("write");
			exit(EXIT_FAILURE);
		}
		b += n;
		buflen -= n;
	}
}

void readall(int fd, void *buf, size_t buflen) {
	char *b = buf;
	while (buflen > 0) {
		ssize_t n = read(fd, b, buflen);
		if (n < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		b += n;
		buflen -= n;
	}
}
