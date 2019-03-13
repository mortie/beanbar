#include "providers.h"

#include <stdlib.h>
#include <stdio.h>
#include <pulse/pulseaudio.h>
#include <math.h>

#include "src/ipc.h"

static void sink_info_callback(
		pa_context *c, const pa_sink_info *i, int eol, void *data) {
	if (eol) return;
	double volume = (double)pa_cvolume_avg(&(i->volume)) / (double)PA_VOLUME_NORM;
	int percent = round(volume * 100.0);
	ipc_sendf("%s:%s:%i:%i", i->name, i->description, percent, i->mute);
}

static void subscribe_callback(
		pa_context *ctx, pa_subscription_event_type_t type, uint32_t idx, void *data) {

	type &= PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
	if (type != PA_SUBSCRIPTION_EVENT_SINK) {
		fprintf(stderr, "Unexpected event type: %i\n", type);
		return;
	}

	pa_context_get_sink_info_by_index(ctx, idx, sink_info_callback, NULL);
}

void state_callback(pa_context *ctx, void *data) {
	switch (pa_context_get_state(ctx)) {
	case PA_CONTEXT_FAILED:
		fprintf(stderr, "Connecting failed.\n");
		exit(EXIT_FAILURE);
		break;

	case PA_CONTEXT_TERMINATED:
		fprintf(stderr, "Terminated.\n");
		exit(EXIT_SUCCESS);
		break;

	case PA_CONTEXT_READY:
		pa_context_set_subscribe_callback(ctx, subscribe_callback, NULL);
		pa_context_subscribe(ctx, PA_SUBSCRIPTION_MASK_SINK, NULL, NULL);

		pa_context_get_sink_info_list(ctx, sink_info_callback, NULL);
		break;

	default: break;
	}
}

static int main(int argc, char **argv) {
	pa_mainloop *m = pa_mainloop_new();
	pa_mainloop_api *mainloop_api = pa_mainloop_get_api(m);
	pa_signal_init(mainloop_api);

	pa_context *ctx = pa_context_new(mainloop_api, "webbar-stats");
	if (ctx == NULL) {
		fprintf(stderr, "pa_context_new() failed.\n");
		return EXIT_FAILURE;
	}

	pa_context_set_state_callback(ctx, state_callback, NULL);

	if (pa_context_connect(ctx, NULL, 0, NULL) < 0) {
		fprintf(stderr, "pa_context_connect() failed: %s\n", pa_strerror(pa_context_errno(ctx)));
		return EXIT_FAILURE;
	}

	int ret;
	if (pa_mainloop_run(m, &ret) < 0) {
		fprintf(stderr, "pa_mainloop_run() failed.\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

struct prov prov_audio = { "audio", main };
