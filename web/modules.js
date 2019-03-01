window.Label = class Label extends Component {
	render(props, state) {
		return h("module", null, props.text);
	}
};
