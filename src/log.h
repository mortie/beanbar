#ifndef LOG_H
#define LOG_H

extern int log_debug;

#define debug(...) do { \
	if (log_debug) { \
		fprintf(stderr, "(debug) " __VA_ARGS__); \
		fprintf(stderr, "\n"); \
	} \
} while (0)

#define warn(...) do { \
	fprintf(stderr, "(warn) " __VA_ARGS__); \
	fprintf(stderr, "\n"); \
} while (0)

#endif
