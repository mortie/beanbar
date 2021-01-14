class Group extends ModComponent {
	render(props, state) {
		return this.el(null, props.children);
	}

	css() {
		return `
		module.Group {
			padding: initial;
			background: initial;
			outline: initial;
		}`;
	}
}

class Label extends ModComponent {
	render(props, state) {
		return this.el(null, props.text);
	}
};

class Launcher extends ModComponent {
	render(props, state) {
		return this.el({
			className: "clickable",
			onClick: () => new IPCProc(props.cmd) },
			props.text || props.cmd);
	}
}

class Drawer extends ModComponent {
	constructor() {
		super();
		this.timeout = null;
	}

	keepOpen() {
		if (this.props.timeout != -1) {
			if (this.timeout != null)
				clearTimeout(this.timeout);
			this.timeout = setTimeout(() => this.close(), this.props.timeout);
		}
	}

	open() {
		this.setState({ opened: true });
		this.keepOpen();
	}

	close() {
		this.setState({ opened: false });
		if (this.timeout) {
			clearTimeout(this.timeout);
			this.timeout = null;
		}
	}

	render(props, state) {
		let className = state.opened ? "open" : "closed";
		if (state.opened) {
			return this.el({ className, onClick: this.keepOpen.bind(this) },
				h(Group, null, props.children),
				h("div", {
					className: "tray-toggle clickable",
					onClick: () => this.close() }));
		} else {
			return this.el({ className },
				h("div", {
					className: "tray-toggle clickable",
					onClick: () => this.open() }));
		}
	}

	css() {
		return `
		module.Drawer {
			position: relative;
			padding: 0px;
		}
		module.Drawer > module.Group {
			position: absolute;
			right: var(--min-width);
		}
		module.Drawer > .tray-toggle {
			cursor: pointer;
			width: var(--min-width);
			text-align: center;
			outline: 1px solid var(--col-outline);
		}
		module.Drawer.closed > .tray-toggle::after {
			content: "◂";
		}
		module.Drawer.open > .tray-toggle::after {
			content: "✕";
		}`;
	}
}
Drawer.defaultProps = { timeout: 10000 };

class Audio extends ModComponent {
	componentDidMount() {
		this.state.sinks = {};
		this.setState();

		let proc = this.ipcProc("beanbar-stats audio", msg => {
			let parts = msg.split(":");
			let evt = parts[0];
			if (evt == "change") {
				let [ evt, id, desc, vol, muted ] = parts;
				this.state.sinks[id] = { desc, vol, muted: muted == "1" };
			} else if (evt == "remove") {
				let [ evt, id ] = parts;
				delete this.state.sinks[id];
			}
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
			els.push(h("div", { className, title: sink.desc },
				h(SliderWidget, { val: parseInt(sink.vol), text })));
		}

		let className = els.length != 0 ? "filled" : "";
		return this.el({ className }, `Vol: `, els);
	}

	css() {
		return `
		module.Audio.filled {
			padding-right: 0px;
		}
		module.Audio div.muted widget.SliderWidget .inner {
			display: none;
		}`;
	}
}

class Disk extends ModComponent {
	componentDidMount() {
		let proc = this.ipcProc(IPC_EXEC_SH, `
		read -r mount
		while read _; do
			df -h "$mount" | grep -v "Avail" | awk '{print $4}'
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
		let proc = this.ipcProc(IPC_EXEC_SH, `
		read -r bat
		path="/sys/class/power_supply/$bat"
		full="$(cat "$path/charge_full")"
		while read _; do
			now="$(cat "$path/charge_now")"
			state="$(cat "$path/status")"
			echo "$state:$(echo "($now / $full) * 100" | bc -l)"
		done
		`, msg => {
			let state = msg.split(":")[0];
			let percent = Math.round(parseFloat(msg.split(":")[1]));
			this.setState({ state, percent });
		});
		proc.send(this.props.battery);

		onUpdate(() => proc.send());
	}

	render(props, state) {
		if (state.percent == null)
			return;

		let className = state.state.toLowerCase();
		if (state.percent <= props.low) className += " low";

		return this.el(null,
			h("span", null, "Bat: "),
			h("span", { className }, `${state.percent}%`));
		return this.el(null, `Bat: ${state.percent}%`);
	}

	css() {
		return `
		module.Battery .low {
			color: var(--col-urgent);
		}
		module.Battery .low.charging {
			color: var(--col-warning);
		}`;
	}
}
Battery.defaultProps = { low: 15, battery: "BAT0" };

class Network extends ModComponent {
	componentDidMount() {
		this.state.connections = {};
		this.setState();

		let proc = this.ipcProc("beanbar-stats network", msg => {
			if (msg == "reset") {
				this.state.connections = {};
			} else {
				let [ path, state, name ] = msg.split(":");

				if (state == "DISCONNECTED" || state == "DISCONNECTING") {
					if (this.state.connections[path])
						delete this.state.connections[path];
				} else {
					this.state.connections[path] = { name, state };
				}
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
		let proc = this.ipcProc(IPC_EXEC_SH, `
		while read _; do
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
		let proc = this.ipcProc("beanbar-stats processor", msg =>
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

		this.i3msg = this.ipcProc(IPC_EXEC_SH, `
		while read -r cmd; do
			i3-msg "$cmd"
		done
		`, msg => {});

		let proc = this.ipcProc("beanbar-stats i3workspaces", this.onMsg.bind(this));

		if (this.props.scroll) {
			let scrollLim = 150;
			let minScroll = 10;
			let recentThresh = 100;
			let acc = 0;
			let recent = false;
			let touchMoved = false;
			let clearTO = null;

			document.body.addEventListener("touchmove", () => touchMoved = true);
			document.body.addEventListener("touchend", () => touchMoved = true);

			document.body.addEventListener("wheel", evt => {
				if (touchMoved) {
					acc = 0;
					touchMoved = false;
					return;
				}

				if (clearTO)
					clearTimeout(clearTO);

				clearTO = setTimeout(() => {
					acc = 0;
					recent = false;
					clearTO = null;
				}, recentThresh);

				acc += evt.deltaY;
				let trigger =
					evt.deltaY >= 10 ||
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
			this.setState();
		} else if (type == "0x80000000") {
			if (data.change == "focus") {
				if (data.old)
					this.state.workspaces[data.old.num] = data.old;
				this.state.workspaces[data.current.num] = data.current;
				this.state.workspaces[data.current.num].urgent = false;
				this.state.workspaces[data.current.num].focused = true;
				this.state.visible[data.current.output] = data.current.num;
				this.setState();
			} else if (data.change == "init") {
				if (data.old)
					this.state.workspaces[data.old.num] = data.old;
				this.state.workspaces[data.current.num] = data.current;
				this.setState();
			} else if (data.change == "rename") {
				this.state.workspaces[data.current.num] =
					this.state.workspaces[this.state.visible[data.current.output]];
				this.state.workspaces[data.current.num].name = data.current.name;
				this.state.workspaces[data.current.num].num = data.current.num;
				delete this.state.workspaces[this.state.visible[data.current.output]];
				this.state.visible[data.current.output] = data.current.num;
				this.setState();
			} else if (data.change == "empty") {
				delete this.state.workspaces[data.current.num];
				this.setState();
			} else if (data.change == "urgent") {
				this.state.workspaces[data.current.num].urgent = data.current.urgent;
				this.setState();
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
			color: var(--col-highlight);
		}
		module.I3Workspaces .workspace {
			display: inline-block;
			position: relative;
			padding: 0px 4px;
			min-width: var(--min-width);
			text-align: center;
			background: var(--col-bg-3);
		}
		module.I3Workspaces .workspace.urgent {
			color: var(--col-urgent);
			font-weight: bold;
		}
		module.I3Workspaces .workspace.focused::after {
			position: absolute;
			content: " ";
			background: var(--col-highlight);
			width: 100%;
			height: 3px;
			left: 0px;
			bottom: 0px;
		}
		module.I3Workspaces .workspace.visible.mon-0 {
			background: var(--col-nice-1);
		}
		module.I3Workspaces .workspace.visible.mon-1 {
			background: var(--col-nice-2);
		}
		module.I3Workspaces .workspace.visible.mon-2 {
			background: var(--col-nice-3);
		}
		module.I3Workspaces .workspace.visible.mon-3 {
			background: var(--col-nice-4);
		}`;
	}
}
I3Workspaces.defaultProps = { numMons: 4, scroll: false };
