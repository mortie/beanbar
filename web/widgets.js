class Label extends ModComponent {
	render(props, state) {
		return this.el(props.text);
	}
};

class Battery extends ModComponent {
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
		`, msg => this.setState({ percent: Math.round(parseFloat(msg)) }));

		onUpdate(() => proc.send("\n"));
	}

	render(props, state) {
		return this.el(`Bat: ${state.percent}%`);
	}
}

class Wireless extends ModComponent {
	constructor() {
		super();
		this.setState({ connection: "--" });
	}

	componentDidMount() {
		let proc = new IPCProc(IPC_EXEC_SH, `
			while read; do
				nmcli device status | grep 'wifi\\s\\+connected' | awk '{print $4}'
			done
		`, msg => this.setState({ connection: msg.trim() }));

		onUpdate(() => proc.send("\n"));
	}

	render(props, state) {
		return this.el(`WiFi: ${state.connection}`);
	}
}

class Memory extends ModComponent {
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
		return this.el(`Mem: ${Math.round(fracUsed * 100)}%`);
	}
}

class Processor extends ModComponent {
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
		return this.el(`CPU: ${state.percent}%`);
	}
}

class Time extends ModComponent {
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

class I3Workspaces extends ModComponent {
	constructor() {
		super();
		this.setState({ workspaces: [] });
	}

	componentDidMount() {
		let proc = new IPCProc(IPC_EXEC_PYTHON, `
			import socket
			import sys
			import subprocess
			import struct

			try:
				sockpath = str(subprocess.check_output([ "i3", "--get-socketpath" ]), "utf-8").strip()
			except:
				sockpath = str(subprocess.check_output([ "sway", "--get-socketpath" ]), "utf-8").strip()

			sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
			sock.connect(sockpath)

			magic = b"i3-ipc"

			I3_GET_WORKSPACES = 1
			I3_SUBSCRIBE = 2

			def send(t, payload):
				if payload == None:
					msg = magic + struct.pack("=II", 0, t)
				else:
					msg = magic + struct.pack("=II", len(payload), t) + bytes(payload, "utf-8")
				sock.sendall(msg)

			def recv():
				fmt = "=" + str(len(magic)) + "xII"
				head = sock.recv(struct.calcsize(fmt))
				l, t = struct.unpack(fmt, head)
				payload = sock.recv(l)
				return payload

			send(I3_GET_WORKSPACES, None)
			sys.stdout.buffer.write(b"workspaces:" + recv() + b"\\n")
			sys.stdout.flush()

			send(I3_SUBSCRIBE, '["workspace"]')
			resp = recv()
			if resp != b'{"success":true}':
				raise Exception("Failed to subscribe for workspace events: " + str(resp))

			while True:
				payload = recv()
				sys.stdout.buffer.write(b"event:" + payload + b"\\n")
				sys.stdout.flush()
		`, this.onMsg.bind(this));
	}

	onMsg(msg) {
		if (msg.startsWith("workspaces:")) {
			msg = msg.substr("workspaces:".length);
			let arr = JSON.parse(msg);
			this.state.workspaces = [];
			arr.forEach(ws => this.state.workspaces[ws.num] = ws);
			this.setState(this.state);
		} else if (msg.startsWith("event:")) {
			msg = msg.substr("event:".length);
			let evt = JSON.parse(msg);
			if (evt.change == "focus") {
				this.state.workspaces[evt.old.num] = evt.old;
				this.state.workspaces[evt.old.num].focused = false;
				this.state.workspaces[evt.current.num] = evt.current;
				this.state.workspaces[evt.current.num].focused = true;
				this.setState(this.state);
			} else if (evt.change == "empty") {
				delete this.state.workspaces[evt.current.num];
				this.setState(this.state);
			}
		}
	}

	render(props, state) {
		return this.el(
			state.workspaces.map(ws => {
				let className = "workspace ";
				if (ws.focused) className += "focused ";
				if (ws.urgent) className += "urgent ";
				return h("div", { className }, ws.name);
			}));
	}
}
