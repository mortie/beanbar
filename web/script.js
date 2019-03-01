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

ipcSend("exec", `
	sh
	i=0
	while :; do
		echo hello $i; i=$(($i + 1))
		sleep 2
	done`, function(msg) {
		console.log("thing 1 got:", msg);
	});

ipcSend("exec", `
	sh
	i=0
	while :; do
		echo bye $i
		i=$(($i + 1))
		sleep 2
	done`, function(msg) {
		console.log("thing 2 got:", msg);
	});

window.onIPCMessage = function(id, msg) {
	ipcCbs[id](msg.trim());
}

})();
