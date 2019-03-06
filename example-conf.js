config({
	updateTime: 3000,
});

init(
	h(I3Workspaces),
	h("group", null,
		h(Wireless),
		h(Battery),
		h(Memory),
		h(Processor),
		h(Time),
	),
);
