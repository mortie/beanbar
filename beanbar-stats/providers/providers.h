#ifndef PROVIDERS_H
#define PROVIDERS_H

struct prov {
	const char *name;
	int (*main)(int argc, char **argv);
};

#define providers() \
	X(prov_processor) \
	X(prov_i3workspaces) \
	X(prov_wireless) \
	X(prov_audio)

#define X(x) extern struct prov x;
providers()
#undef X

#endif
