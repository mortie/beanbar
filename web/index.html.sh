#!/bin/sh
cd "$(dirname "$0")"
cat <<EOF
<!DOCTYPE html>
<html>
	<head>
		<meta charset="utf-8">
		<title>Beanbar</title>
		<style>
$(cat style.css)
		</style>
	</head>
	<body>
		<script>
$(cat vendor/preact.js)
$(cat lib.js)
$(cat ctx.js)
$(cat widgets.js)
$(cat modules.js)
$(cat themes.js)

fetch("local:conf").then(res => res.text()).then(js => {
	let script = document.createElement("script");
	script.text = "(async function() {" + js + "})();";
	document.getElementsByTagName("head")[0].appendChild(script);
});
		</script>
	</body>
</html>
EOF
