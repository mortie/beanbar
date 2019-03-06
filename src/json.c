#include "json.h"

#include <gtk/gtk.h>

static char hexchar(char c) {
	if (c <= 9)
		return '0' + c;
	else
		return 'a' + (c - 10);
}

char *json_stringify_string(const char *input, size_t *len) {
	size_t allocd = 32;
	*len = 0;
	char *output = g_malloc(allocd);

	output[(*len)++] = '"';

	while (*input) {
		if (allocd <= *len + 8) {
			allocd *= 2;
			output = g_realloc(output, allocd);
		}

		if (*input < 0x20) {
			output[(*len)++] = '\\';
			output[(*len)++] = 'u';
			output[(*len)++] = '0';
			output[(*len)++] = '0';
			output[(*len)++] = hexchar((*input & 0xf0) >> 4);
			output[(*len)++] = hexchar(*input & 0x0f);
		} else if (*input == '\\') {
			output[(*len)++] = '\\';
			output[(*len)++] = '\\';
		} else if (*input == '"') {
			output[(*len)++] = '\\';
			output[(*len)++] = '"';
		} else {
			output[(*len)++] = *input;
		}

		input += 1;
	}

	output[(*len)++] = '"';
	output[(*len)++] = '\0';

	return output;
}
