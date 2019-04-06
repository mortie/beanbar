#include "providers.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "src/ipc.h"

struct snapshot {
	int total;
	int idle;
};

int read_snapshot(struct snapshot *snap) {
	FILE *f = fopen("/proc/stat", "r");
	int vals[7];
	int ret = fscanf(f, "cpu  %i %i %i %i %i %i %i",
		vals, vals + 1, vals + 2, vals + 3, vals + 4, vals + 5, vals + 6);
	if (ret != 7) {
		fprintf(stderr, "fscanf returned %i, expected 7\n", ret);
		fclose(f);
		return -1;
	}

	snap->idle = vals[3];
	snap->total = vals[0] + vals[1] + vals[2] + vals[3] + vals[4] + vals[5] + vals[6];
	fclose(f);
	return 0;
}

static int main(int argc, char **argv) {
	(void)argc; (void)argv;

	struct snapshot prev;
	if (read_snapshot(&prev) < 0)
		abort();
	while (ipc_loop()) {
		struct snapshot now;
		if (read_snapshot(&now) < 0)
			continue;

		double deltatotal = now.total - prev.total;
		double deltaidle = now.idle - prev.idle;
		if (deltatotal == 0.0) {
			ipc_send("0");
		} else {
			double usage = 1.0 - (deltaidle / deltatotal);
			ipc_sendf("%i", (int)(usage * 100.0));
		}
		memcpy(&prev, &now, sizeof(prev));
	}

	return EXIT_SUCCESS;
}

struct prov prov_processor = { "processor", main };
