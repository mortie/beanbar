config({
	updateTime: 2000,
});

class Test extends ModComponent {
	constructor() {
		super();
		this.state.val = 10;
	}

	render(props, state) {
		return this.el(null, h(SliderWidget, {
			change: val => this.setState({ val }),
			val: state.val }));
	}
}

init(
	h(I3Workspaces, { scroll: true }),
	h("group", null,
		h(Test),
		h(Audio),
		h(Disk),
		h(Network),
		h(Battery),
		h(Memory),
		h(Processor),
		h(Time),
	),
);
