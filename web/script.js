(function() {

let { Component, h, render } = preact;
window.Component = Component;
window.h = h;

let updateCbs = [];
function onUpdate(cb) {
	updateCbs.push(cb);
}

window.init = function init(modules) {
	class App extends Component {
		render() {
			function renderMod(mod) {
				if (mod instanceof Array) {
					return h("div", null, ...mod.map(renderMod));
				} else {
					return mod;
				}
			}

			return renderMod(modules);
		}
	}

	render(h(App), document.body);

	function update() {
		updateCbs.forEach(cb => cb());
		setTimeout(update, 1000);
	}
	update();
};

window.Label = class Label extends Component {
	componentDidMount() {
		this.setState({ count: 0 });
		onUpdate(() => this.setState({ count: this.state.count + 1 }));
	}

	render(props, state) {
		return h("div", null, props.text+": "+state.count);
	}
};

window.webkit.messageHandlers.ipc.postMessage({
	type: "no",
	id: 10,
	msg: "Hello World",
});

window.onIPCMessage = function({ id, msg }) {
	console.log("ID:", id, "msg:", msg);
}

})();
