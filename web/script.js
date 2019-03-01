(function() {

let { Component, h, render } = preact;
window.Component = Component;
window.h = h;

let updateCbs = [];
function onUpdate(cb) {
	updateCbs.push(cb);
}

window.init = function init(...modules) {
	render(h('group', null, ...modules), document.body);

	document.body.addEventListener("resize", () =>
		document.body.style.lineHeight = document.body.clientHeight+"px");
	document.body.style.lineHeight = document.body.clientHeight+"px";

	function update() {
		updateCbs.forEach(cb => cb());
		setTimeout(update, 1000);
	}
	update();
};

window.Label = class Label extends Component {
	render(props, state) {
		return h("module", null, props.text);
	}
};

window.IPC_EXEC_SH = "sh";
window.IPC_EXEC_PYTHON = "python3"
window.IPC_EXEC_C = "f=$(mktemp) && cc -o \"$f\" -xc - && \"$f\"; rm -f \"$f\"";

let ipcId = 0;
let ipcCbs = [];
window.ipcSend = function ipcSend(type, msg, cb) {
	ipcCbs[ipcId] = cb;
	window.webkit.messageHandlers.ipc.postMessage({
		type: type,
		msg: msg.trim(),
		id: ipcId++
	});
}

window.onIPCMessage = function(id, msg) {
	ipcCbs[id](msg.trim());
}

})();
