(function() {

let { Component, VNode, h, render } = preact;
window.Component = Component;
window.h = h;

let updates = [];
window.onUpdate = function(cb) {
	updates.push(cb);
}

let conf = {
	updateTime: 5000,
};

function confval(c, name) {
	if (c[name] !== undefined)
		conf[name] = c[name];
}

window.init = function init(c, ...modules) {
	confval(c, "updateTime");

	render(h('group', null, ...modules), document.body);

	document.body.addEventListener("resize", () =>
		document.body.style.lineHeight = document.body.clientHeight+"px");
	document.body.style.lineHeight = document.body.clientHeight+"px";

	function update() {
		updates.forEach(cb => cb());
	}
	update();
	setInterval(update, conf.updateTime);
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

window.onIPCMessage = function(id, msg) {
	ipcCbs[id](msg.trim());
}

let ipcId = 0;
window.IPCProc = class IPCProc {
	constructor(first, msg, cb) {
		this.id = ipcId++;
		window.webkit.messageHandlers.ipc.postMessage({
			type: "exec",
			id: this.id,
			msg: first+"\n"+fixIndent(msg),
		});
		ipcCbs[this.id] = cb;
	}

	send(msg) {
		window.webkit.messageHandlers.ipc.postMessage({
			type: "write",
			id: this.id,
			msg: msg,
		});
	}
}

})();
