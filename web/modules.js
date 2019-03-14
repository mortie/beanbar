class Label extends ModComponent {
	render(props, state) {
		return this.el(null, props.text);
	}
};

class Audio extends ModComponent {
	componentDidMount() {
		this.state.sinks = {};
		this.setState();

		let proc = new IPCProc("beanbar-stats audio", msg => {
			let [ name, desc, vol, muted ] = msg.split(":");
			this.state.sinks[name] = { desc, vol, muted: muted == "1" };
			this.setState({ sinks: this.state.sinks });
		});
	}

	render(props, state) {
		if (state.sinks == null)
			return;

		let els = [];
		for (let key of Object.keys(state.sinks)) {
			let sink = state.sinks[key];
			let className = "";
			let icon = sink.muted ? "🔇" : "🔊";
			let text = `${icon} ${sink.vol}%`;
			if (sink.muted) className += "muted ";
			els.push(h("div", { className },
				h(SliderWidget, { val: parseInt(sink.vol), text })));
		}

		return this.el(null, `Vol: `, els);
	}

	css() {
		return `
		module.Audio div.muted widget.SliderWidget .inner {
			display: none;
		}
		module.Network div:not(:last-child):after {
			content: ", ";
		}`;
	}
}

class Disk extends ModComponent {
	componentDidMount() {
		let proc = new IPCProc(IPC_EXEC_SH, `
		read -r mount
		while read; do
			df -h "$mount" | awk '{print $4}'
		done
		`, msg => this.setState({ avail: msg }));
		proc.send(this.props.mount);

		onUpdate(() => proc.send());
	}

	render(props, state) {
		if (state.avail == null)
			return;

		return this.el(null, `${props.mount}: ${state.avail}`);
	}
}
Disk.defaultProps = { mount: "/" };

class Battery extends ModComponent {
	componentDidMount() {
		let proc = new IPCProc(IPC_EXEC_SH, `
		full="$(cat /sys/class/power_supply/BAT0/charge_full)"
		while read; do
			now="$(cat /sys/class/power_supply/BAT0/charge_now)"
			echo "($now / $full) * 100" | bc -l
		done
		`, msg => this.setState({ percent: Math.round(parseFloat(msg)) }));

		onUpdate(() => proc.send());
	}

	render(props, state) {
		if (state.percent == null)
			return;

		return this.el(null, `Bat: ${state.percent}%`);
	}
}

class Network extends ModComponent {
	componentDidMount() {
		this.state.connections = {};
		this.setState();

		let proc = new IPCProc("beanbar-stats network", msg => {
			let [ path, state, name ] = msg.split(":");

			if (state == "DISCONNECTED" || state == "DISCONNECTING") {
				if (this.state.connections[path])
					delete this.state.connections[path];
			} else {
				this.state.connections[path] = { name, state };
			}

			this.setState();
		});
	}

	render(props, state) {
		if (state.connections == null)
			return;

		let els = [];
		for (let key of Object.keys(state.connections)) {
			let conn = state.connections[key];
			if (conn.state == "CONNECTING")
				els.push(h("div", null, conn.name, " ", h(LoadingWidget)));
			else if (conn.state == "CONNECTED")
				els.push(h("div", null, conn.name));
		}

		return this.el(null, "Net: ", els);
	}

	css() {
		return `
		module.Network div:not(:last-child):after {
			content: ', ';
		}`;
	}
}

class Memory extends ModComponent {
	componentDidUpdate() { this.consistentWidth(); }

	componentDidMount() {
		let proc = new IPCProc(IPC_EXEC_SH, `
		while read; do
			free | grep '^Mem' | awk  '{print $2 " " $7}'
		done
		`, msg => this.setState({ parts: msg.split(" ") }));

		onUpdate(() => proc.send());
	}

	render(props, state) {
		if (state.parts == null)
			return;

		let total = parseInt(state.parts[0]);
		let available = parseInt(state.parts[1]);
		let percent = Math.round((1 - (available / total)) * 100);
		return this.el(null,
			h("span", null, "Mem: "),
			h("span", null, `${percent}%`));
	}
}

class Processor extends ModComponent {
	componentDidUpdate() { this.consistentWidth(); }

	componentDidMount() {
		let proc = new IPCProc("beanbar-stats processor", msg =>
			this.setState({ percent: msg }));

		onUpdate(() => proc.send());
	}

	render(props, state) {
		if (state.percent == null)
			return;

		return this.el(null,
			h("span", null, "CPU: "),
			h("span", null, `${state.percent}%`));
	}
}

class Time extends ModComponent {
	componentDidMount() {
		onUpdate(() => this.setState({ now: new Date() }));
	}

	defaultFmt(d) {
		return d.toDateString()+", "+
			d.getHours().toString().padStart(2, "0")+":"+
			d.getMinutes().toString().padStart(2, "0");
	}

	render(props, state) {
		if (state.now == null)
			return;

		let func = props.func || this.defaultFmt;
		return this.el(null, func(state.now));
	}
}

class I3Workspaces extends ModComponent {
	constructor() {
		super();
		this.state = { mons: {}, monId: 0, visible: {} };
	}

	componentDidMount() {
		this.setState({ mode: "default" });

		this.i3msg = new IPCProc(IPC_EXEC_SH, `
		while read -r cmd; do
			i3-msg "$cmd"
		done
		`, msg => {});

		let proc = new IPCProc("beanbar-stats i3workspaces", this.onMsg.bind(this));

		if (this.props.scroll) {
			let scrollLim = 150;
			let minScroll = 10;
			let recentThresh = 100;
			let acc = 0;
			let recent = false;
			let clearTO = null;
			document.body.addEventListener("wheel", evt => {
				if (clearTO) clearTimeout(clearTO);
				clearTO = setTimeout(() => {
					acc = 0;
					recent = false;
					clearTO = null;
				}, recentThresh);

				acc += evt.deltaY;
				let trigger =
					Math.abs(acc) > scrollLim ||
					(Math.abs(acc) > minScroll && !recent);
				if (!trigger)
					return;

				if (acc > 0)
					this.nextWorkspace();
				else if (acc < 0)
					this.prevWorkspace();
				acc = 0;
				recent = true;
			});
		}
	}

	nextWorkspace() {
		let current = this.state.workspaces.find(ws => ws && ws.focused);
		if (!current) return;
		let next;
		for (let n = current.num + 1; n < this.state.workspaces.length; ++n) {
			if (this.state.workspaces[n]) {
				next = this.state.workspaces[n];
				break;
			}
		}
		if (!next) return;

		this.i3msg.send("workspace number "+next.num);
	}

	prevWorkspace() {
		let current = this.state.workspaces.find(ws => ws && ws.focused);
		if (!current) return;
		let prev;
		for (let n = current.num - 1; n >= 0; --n) {
			if (this.state.workspaces[n]) {
				prev = this.state.workspaces[n];
				break;
			}
		}
		if (!prev) return;

		this.i3msg.send("workspace number "+prev.num);
	}

	onMsg(msg) {
		let type = msg.split(":")[0];
		let json = msg.substr(type.length + 1);
		let data;
		try {
			data = JSON.parse(json);
		} catch (err) {
			console.log("Failed to parse JSON:", err);
			console.log(msg);
			return;
		}

		if (type == "0x1") {
			this.state.workspaces = [];
			data.forEach(ws => {
				this.state.workspaces[ws.num] = ws;
				if (ws.visible)
					this.state.visible[ws.output] = ws.num;
			});
			this.setState(this.state);
		} else if (type == "0x80000000") {
			if (data.change == "focus") {
				if (data.old) {
					this.state.workspaces[data.old.num] = data.old;
				}
				this.state.workspaces[data.current.num] = data.current;
				this.state.workspaces[data.current.num].urgent = false;
				this.state.workspaces[data.current.num].focused = true;
				this.state.visible[data.current.output] = data.current.num;
				this.setState(this.state);
			} else if (data.change == "empty") {
				delete this.state.workspaces[data.current.num];
				this.setState(this.state);
			} else if (data.change == "urgent") {
				this.state.workspaces[data.current.num].urgent = data.current.urgent;
				this.setState(this.state);
			}
		} else if (type == "0x80000002") {
			this.setState({ mode: data.change });
		}
	}

	render(props, state) {
		if (state.workspaces == null)
			return;

		let workspaces = state.workspaces.map(ws => {
			if (state.mons[ws.output] == null)
				state.mons[ws.output] = state.monId++ % props.numMons;

			let className = `workspace clickable mon-${state.mons[ws.output]} `;
			if (state.visible[ws.output] == ws.num) className += "visible ";
			if (ws.focused) className += "focused ";
			if (ws.urgent) className += "urgent ";
			return h("div", {
				className,
				onclick: () => this.i3msg.send("workspace number "+ws.num),
			}, ws.name);
		});

		if (state.mode == "default") {
			return this.el(null, workspaces);
		} else {
			return this.el(null,
				workspaces,
				h("div", { className: "mode" }, state.mode));
		}
	}

	css() {
		return `
		module.I3Workspaces {
			padding: 0px;
		}
		module.I3Workspaces .mode {
			padding: 0px 8px;
			color: darkred;
		}
		module.I3Workspaces .workspace {
			display: inline-block;
			position: relative;
			padding: 0px 4px;
			min-width: 32px;
			text-align: center;
			background: #bbb;
		}
		module.I3Workspaces .workspace.urgent {
			color: red;
			font-weight: bold;
		}
		module.I3Workspaces .workspace.focused::after {
			position: absolute;
			content: " ";
			background: darkred;
			width: 100%;
			height: 3px;
			left: 0px;
			bottom: 0px;
		}
		module.I3Workspaces .workspace.visible.mon-0 {
			background: #75b6cc;
		}
		module.I3Workspaces .workspace.visible.mon-1 {
			background: #b675cc;
		}
		module.I3Workspaces .workspace.visible.mon-2 {
			background: #cc8b75;
		}
		module.I3Workspaces .workspace.visible.mon-3 {
			background: #8bcc75;
		}`;
	}
}
I3Workspaces.defaultProps = { numMons: 4, scroll: false };
