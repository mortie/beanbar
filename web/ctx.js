(function() {

let { render } = preact;

let updateListeners = [];
window.onUpdate = function onUpdate (cb) {
	updateListeners.push(cb);
};

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

let style = "";
window.css = function css(str) {
	style += str;
}

window.init = function init(...modules) {
	render(h('group', null, ...modules), document.body);

	document.body.addEventListener("resize", () =>
		document.body.style.lineHeight = document.body.clientHeight+"px");
	document.body.style.lineHeight = document.body.clientHeight+"px";

	function update() {
		updateListeners.forEach(cb => cb());
	}

	update();
	setInterval(update, conf.updateTime);

	if (style != "") {
		let sheet = document.createElement("style");
		sheet.type = "text/css";
		sheet.innerHTML = style;
		document.getElementsByTagName("head")[0].appendChild(sheet);
	}
};

})();