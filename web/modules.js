class Label extends Component {
	render(props, state) {
		return h("module", null, props.text);
	}
};

class Battery extends Component {
	componentDidMount() {
		let proc = new IPCProc(`
			${IPC_EXEC_SH}
			full="$(cat /sys/class/power_supply/BAT0/charge_full)"
			while read; do
				now="$(cat /sys/class/power_supply/BAT0/charge_now)"
				echo "($now / $full) * 100" | bc -l
				sleep 2;
			done
		`, msg =>
			this.setState({ percent: parseInt(msg) }));

		onUpdate(() => proc.send("\n"));
	}

	render(props, state) {
		return h("module", null, "Bat: "+state.percent+"%");
	}
}

class Wireless extends Component {
	componentDidMount() {
		let proc = new IPCProc(`
			${IPC_EXEC_SH}
			while read; do
				nmcli device status | grep connected | awk '{print $4}'
			done
		`, msg =>
			this.setState({ connection: msg.trim() }));

		onUpdate(() => proc.send("\n"));
	}

	render(props, state) {
		return h("module", null, state.connection)
	}
}
