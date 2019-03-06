(function() {

let { Component, h } = preact;

window.h = h;

let modulesWithCss = {};

window.ModComponent = class ModComponent extends Component {
	constructor() {
		super();
		this.width = 0;
		if (this.css && !modulesWithCss[this.id]) {
			let sheet = document.createElement("style");
			sheet.type = "text/css";
			sheet.appendChild(document.createTextNode(this.css()));
			document.getElementsByTagName("head")[0].appendChild(sheet);
			modulesWithCss[this.id] = true;
		}
	}

	el(...args) {
		return h("module", { className: this.id() }, ...args);
	}

	consistentWidth() {
		if (this.base.offsetWidth > this.width) {
			this.width = this.base.offsetWidth;
			this.base.style.minWidth = this.width+"px";
		}
	}

	id() {
		return this.constructor.name;
	}
};

window.IPC_EXEC_SH = `sh "$TMPFILE"`;
window.IPC_EXEC_PYTHON = `python3 "$TMPFILE"`
window.IPC_EXEC_C = `cc -o "$TMPFILE.bin" -xc "$TMPFILE" && "$TMPFILE.bin"; rm -f "$TMPFILE.bin"`;

function fixIndent(str) {
	let lines = str.split("\n");
	while (lines.length > 0 && lines[0].trim() == "")
		lines.shift();
	if (lines.length == 0)
		return str;

	let indents = lines[0].match(/^\s*/)[0];
	let fixed = "";
	for (let line of lines) {
		fixed += line.replace(indents, "")+"\n";
	}
	return fixed;
}

let ipcCbs = [];
let ipcId = 0;

window.onIPCMessage = function(id, msg) {
	ipcCbs[id](msg);
};

window.IPCProc = class IPCProc {
	constructor(first, msg, cb) {
		this.buf = "";
		this.id = ipcId++;
		window.webkit.messageHandlers.ipc.postMessage({
			type: "exec",
			id: this.id,
			msg: first+"\n"+fixIndent(msg),
		});
		ipcCbs[this.id] = this.onMsg.bind(this)
		this.cb = cb;
	}

	onMsg(msg) {
		let parts = msg.split("\n");
		for (let i = 0; i < parts.length - 1; ++i) {
			try {
				this.cb(this.buf + parts[i]);
			} catch (err) {
				console.error(err);
			}
			this.buf = "";
		}
		this.buf += parts[parts.length - 1];
	}

	send(msg = "") {
		window.webkit.messageHandlers.ipc.postMessage({
			type: "write",
			id: this.id,
			msg: msg+"\n",
		});
	}
};

})();
