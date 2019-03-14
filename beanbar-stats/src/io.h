#ifndef IO_H
#define IO_H

#include <stdlib.h>

void writeall(int fd, const void *buf, size_t buflen);
void readall(int fd, void *buf, size_t buflen);

#endif
