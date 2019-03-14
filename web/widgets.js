class LoadingWidget extends WidgetComponent {
	render() {
		return this.el(null);
	}

	css() {
		return `
		widget.LoadingWidget {
			vertical-align: top;
			display: inline-block;
			height: 50vh;
			width: 50vh;
			margin-top: calc(25vh - 1px);
		}
		widget.LoadingWidget::after {
			box-sizing: border-box;
			content: " ";
			display: block;
			height: 50vh;
			width: 50vh;
			margin: 1px;
			border-radius: 50%;
			border: 3px solid;
			border-color: #000 transparent #000 transparent;
			animation: LoadingWidget 1.2s linear infinite;
		}
		@keyframes LoadingWidget {
			0% {
				transform: rotate(0deg);
			}
			100% {
				transform: rotate(360deg);
			}
		}`;
	}
}

class SliderWidget extends WidgetComponent {
	onMouse(evt, cb) {
		if (cb == null)
			return;

		evt.preventDefault();
		let e = evt;

		if (evt.type == "mousemove" && evt.buttons != 1)
			return;

		else if (evt.type == "touchmove")
			e = evt.touches[0];

		let outer = evt.target;
		while (outer.className != "outer")
			outer = outer.parentElement;

		let left = outer.offsetLeft;
		let width = outer.offsetWidth;
		let frac = (e.clientX - left) / width;
		if (frac < 0.03)
			frac = 0;
		else if (frac > 0.97)
			frac = 1;

		cb(frac * (this.props.max - this.props.min) + this.props.min, evt);
	}

	onWheel(evt, cb) {
		evt.stopPropagation();

		if (cb == null)
			return;

		let delta = evt.deltaY / 300;
		let scale = this.props.max - this.props.min;
		let frac = (this.props.val - this.props.min) / scale + delta;
		if (frac < 0)
			frac = 0;
		else if (frac > 1)
			frac = 1;

		cb(frac * scale + this.props.min, evt);
	}

	render(props, state) {
		let percent = ((props.val - props.min) / (props.max - props.min)) * 100;
		let text = props.text != null ? props.text : props.val.toFixed(0);
		return this.el(null,
			h("div", {
				className: "outer",
				onClick: evt => this.onMouse(evt, props.onChange),
				onMouseMove: evt => this.onMouse(evt, props.onChange),
				onTouchMove: evt => this.onMouse(evt, props.onChange),
				onWheel: evt => this.onWheel(evt, props.onChange) },
				h("div", {
					className: "inner",
					style: `width: ${percent}%` }),
				h("div", { className: "info" }, text)));
	}

	css() {
		return `
		widget.SliderWidget .outer {
			width: 100px;
			height: 100%;
			position: relative;
			border-radius: 4px;
			box-shadow: 0 4px 5px rgba(0, 0, 0, 0.25) inset;
		}

		widget.SliderWidget .inner {
			border-radius: 4px;
			background: #75b6cc;
			height: 100%;
			max-width: 100%;
		}

		widget.SliderWidget .info {
			position: absolute;
			top: 0px;
			left: 8px;
		}`;
	}
}
SliderWidget.defaultProps = { min: 0, max: 100, val: 10 };
