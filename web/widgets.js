class Label extends Component {
	render(props, state) {
		return h("module", null, props.text);
	}
};

class Battery extends Component {
	constructor() {
		super();
		this.setState({ percent: "?" });
	}

	componentDidMount() {
		let proc = new IPCProc(IPC_EXEC_SH, `
			full="$(cat /sys/class/power_supply/BAT0/charge_full)"
			while read; do
				now="$(cat /sys/class/power_supply/BAT0/charge_now)"
				echo "($now / $full) * 100" | bc -l
				sleep 2;
			done
		`, msg => this.setState({ percent: parseInt(msg) }));

		onUpdate(() => proc.send("\n"));
	}

	render(props, state) {
		return h("module", null, `Bat: ${state.percent}%`);
	}
}

class Wireless extends Component {
	constructor() {
		super();
		this.setState({ connection: "--" });
	}

	componentDidMount() {
		let proc = new IPCProc(IPC_EXEC_SH, `
			while read; do
				nmcli device status | grep connected | awk '{print $4}'
			done
		`, msg => this.setState({ connection: msg.trim() }));

		onUpdate(() => proc.send("\n"));
	}

	render(props, state) {
		return h("module", null, `WiFi: ${state.connection}`);
	}
}

class Memory extends Component {
	constructor() {
		super();
		this.setState({ parts: [ 0, 0 ] });
	}

	componentDidMount() {
		let proc = new IPCProc(IPC_EXEC_SH, `
			while read; do
				free | grep '^Mem' | awk  '{print $2 " " $7}'
			done
		`, msg => this.setState({ parts: msg.split(" ") }));

		onUpdate(() => proc.send("\n"));
	}

	render(props, state) {
		let total = parseInt(state.parts[0]);
		let available = parseInt(state.parts[1]);
		let fracUsed = 1 - (available / total);
		return h("module", null, `Mem: ${Math.round(fracUsed * 100)}%`);
	}
}

class Processor extends Component {
	constructor() {
		super();
		this.setState({ percent: 0 });
	}

	componentDidMount() {
		let proc = new IPCProc(IPC_EXEC_SH, `
			total() {
				echo "$1" | awk '{print $2 + $3 + $4 + $5 + $6 + $7 + $8}'
			}
			idle() {
				echo "$1" | cut -d' ' -f6
			}
			prev="$(cat /proc/stat | head -n 1)"
			while read; do
				now="$(cat /proc/stat | head -n 1)"
				prevtotal="$(total "$prev")"
				previdle="$(idle "$prev")"
				nowtotal="$(total "$now")"
				nowidle="$(idle "$now")"

				deltatotal=$(($nowtotal - $prevtotal))
				deltaidle=$(($nowidle - $previdle))
				echo "(1 - ($deltaidle / $deltatotal)) * 100" | bc -l
				prev="$now"
			done
		`, msg => this.setState({ percent: Math.round(parseFloat(msg)) }));

		onUpdate(() => proc.send("\n"));
	}

	render(props, state) {
		return h("module", null, `CPU: ${state.percent}%`);
	}
}

class Time extends Component {
	constructor() {
		super();
		this.setState({ now: new Date() });
	}

	componentDidMount() {
		onUpdate(() => this.setState({ now: new Date() }));
	}

	render(props, state) {
		if (props.func)
			return h("module", null, props.func(state.now));
		else
			return h("module", null, state.now.toDateString());
	}
}
