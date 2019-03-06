config({
	updateTime: 2000,
});

init(
	h(I3Workspaces, { scroll: true }),
	h("group", null,
		h(Wireless),
		h(Battery),
		h(Memory),
		h(Processor),
		h(Time),
	),
);
