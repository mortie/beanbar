class Label extends Component {
	render(props, state) {
		return h("module", null, props.text);
	}
};

class Battery extends Component {
	componentDidMount() {
		ipcSend("exec", `
			${IPC_EXEC_SH}
			full="$(cat /sys/class/power_supply/BAT0/charge_full)"
			while :; do
				now="$(cat /sys/class/power_supply/BAT0/charge_now)"
				echo "($now / $full) * 100" | bc -l
				sleep 2;
			done
		`, msg => {
			this.setState({ percent: parseInt(msg) });
		});
	}

	render(props, state) {
		return h("module", null, "Bat: "+state.percent+"%");
	}
}
