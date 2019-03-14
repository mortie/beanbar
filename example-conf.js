config({
	updateTime: 2000,
});

init(
	h(I3Workspaces, { scroll: true }),
	h("group", null,
		h(Audio),
		h(Disk),
		h(Network),
		h(Battery),
		h(Memory),
		h(Processor),
		h(Time),
		h(Tray, null,
			h(Launcher, { text: "Firefox", cmd: "blueman-manager" }),
			h(Launcher, { text: "Bluetooth", cmd: "blueman-manager" }),
		),
	),
);
