#ifndef JSON_H
#define JSON_H

#include <stdlib.h>

char *json_escape_string(const char *input, size_t inputlen, size_t *outputlen);

#endif
