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
