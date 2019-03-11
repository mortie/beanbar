#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "providers/providers.h"

static struct prov *providers[] = {
#define X(x) &x,
providers()
#undef X
};

static void usage(const char *argv0) {
	printf("Usage: %s <provider>\n", argv0);
	printf("       %s -l|--list: List all available providers\n", argv0);
	printf("       %s -h|--help: Show this help text\n", argv0);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	size_t provcnt = sizeof(providers) / sizeof(*providers);

	if (strcmp(argv[1], "--list") == 0 || strcmp(argv[1], "-l") == 0) {
		for (size_t i = 0; i < provcnt; ++i) {
			printf("%s\n", providers[i]->name);
		}
		return EXIT_SUCCESS;
	} else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		usage(argv[0]);
		return EXIT_SUCCESS;
	}

	for (size_t i = 0; i < provcnt; ++i) {
		struct prov *prov = providers[i];
		if (strcmp(argv[1], prov->name) == 0) {
			return prov->main(argc - 1, argv + 1);
		}
	}

	fprintf(stderr, "Provider %s not found.\n", argv[1]);
	return EXIT_FAILURE;
}
