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
		`, msg => {
			this.setState({ percent: parseInt(msg) });
		});

		proc.send("\n");
		setInterval(() => proc.send("\n"), 1000);
	}

	render(props, state) {
		return h("module", null, "Bat: "+state.percent+"%");
	}
}
