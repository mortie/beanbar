init({ updateTime: 3000 },
	h(Label, { text: "Hello World" }),
	h("group", null,
		h(Wireless),
		h(Battery),
		h(Memory),
		h(Processor),
		h(Time),
	),
);
