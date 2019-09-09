(function() {

let { render } = preact;

let updateListeners = [];
window.onUpdate = function onUpdate(cb) {
	updateListeners.push(cb);
};

window.triggerUpdate = function triggerUpdate() {
	updateListeners.forEach(cb => cb());
}

window.include = function include(path) {
	return new Promise(resolve => {
		let script = document.createElement("script");
		script.src = path;
		script.onload = resolve;
		document.getElementsByTagName("head")[0].appendChild(script);
	});
}

let conf = {
	updateTime: 5000,
};
window.config = function config(c) {
	for (let i in c) {
		if (conf[i] === undefined)
			console.error("Config key "+i+" doesn't exist");
		else
			conf[i] = c[i];
	}
};

let cssStr = "";
window.css = function css(str) {
	cssStr += str;
}

window.init = function init(...modules) {
	let len = modules.length;
	let style = `grid-template-columns: repeat(${len}, 1fr);`;
	render(h("div", { id: "bar", style }, ...modules), document.body);

	function resize() {
		document.body.style.lineHeight = document.body.clientHeight+"px";
	}
	resize();
	window.addEventListener("resize", resize);

	triggerUpdate();
	setInterval(triggerUpdate, conf.updateTime);

	if (cssStr != "") {
		let sheet = document.createElement("style");
		sheet.type = "text/css";
		sheet.innerText = cssStr;
		document.getElementsByTagName("head")[0].appendChild(sheet);
	}
};

})();
