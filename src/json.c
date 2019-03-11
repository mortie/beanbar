#include "json.h"
#include "log.h"

#include <gtk/gtk.h>

static char hexchar(char c) {
	if (c <= 9)
		return '0' + c;
	else
		return 'a' + (c - 10);
}

char *json_escape_string(const char *input, size_t inputlen, size_t *outputlen) {
	size_t allocd = (inputlen * 1.3) + 8;
	*outputlen = 0;
	char *output = g_malloc(allocd);

	output[(*outputlen)++] = '"';

	while (*input) {
		while (allocd <= *outputlen + 8) {
			debug("reallocing, consider tuning escape_string. (%zi -> %zi, %zi)", allocd, allocd * 2, inputlen);
			allocd *= 2;
			output = g_realloc(output, allocd);
		}

		if (*input == '\\') {
			output[(*outputlen)++] = '\\';
			output[(*outputlen)++] = '\\';
		} else if (*input == '"') {
			output[(*outputlen)++] = '\\';
			output[(*outputlen)++] = '"';
		} else if (*input == '\n') {
			output[(*outputlen)++] = '\\';
			output[(*outputlen)++] = 'n';
		} else if (*input < 0x20) {
			output[(*outputlen)++] = '\\';
			output[(*outputlen)++] = 'u';
			output[(*outputlen)++] = '0';
			output[(*outputlen)++] = '0';
			output[(*outputlen)++] = hexchar((*input & 0xf0) >> 4);
			output[(*outputlen)++] = hexchar(*input & 0x0f);
		} else {
			output[(*outputlen)++] = *input;
		}

		input += 1;
	}

	output[(*outputlen)++] = '"';
	output[(*outputlen)++] = '\0';

	return output;
}
