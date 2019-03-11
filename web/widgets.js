class LoadingWidget extends WidgetComponent {
	render() {
		return this.el(null);
	}

	css() {
		return `
		widget.LoadingWidget,
		widget.LoadingWidget:after {
			border-radius: 50%;
			width: 100vh;
			height: 100vh;
		}
		widget.LoadingWidget {
			margin: 60px auto;
			font-size: 10px;
			position: relative;
			text-indent: -9999em;
			border-top: 1.1em solid rgba(0,0,0, 0.2);
			border-right: 1.1em solid rgba(0,0,0, 0.2);
			border-bottom: 1.1em solid rgba(0,0,0, 0.2);
			border-left: 1.1em solid #000000;
			transform: translateZ(0);
			animation: LoadingWidget 1.1s infinite linear;
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
