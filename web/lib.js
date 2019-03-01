(function() {

let { Component, h, render } = preact;
window.Component = Component;
window.h = h;

let updates = [];
window.onUpdate = function(cb) {
	updates.push(cb);
}

window.init = function init(...modules) {
	render(h('group', null, ...modules), document.body);

	document.body.addEventListener("resize", () =>
		document.body.style.lineHeight = document.body.clientHeight+"px");
	document.body.style.lineHeight = document.body.clientHeight+"px";

	function update() {
		updates.forEach(cb => cb());
	}
	update();
	setInterval(update, 5000);
};

window.IPC_EXEC_SH = `sh "$TMPFILE"`;
window.IPC_EXEC_PYTHON = `python3 "$TMPFILE"`
window.IPC_EXEC_C = `cc -o "$TMPFILE.bin" -xc "$TMPFILE" && "$TMPFILE.bin"; rm -f "$TMPFILE.bin"`;

let ipcCbs = [];

window.onIPCMessage = function(id, msg) {
	ipcCbs[id](msg.trim());
}

let ipcId = 0;
window.IPCProc = class IPCProc {
	constructor(msg, cb) {
		this.id = ipcId++;
		window.webkit.messageHandlers.ipc.postMessage({
			type: "exec",
			id: this.id,
			msg: msg,
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
