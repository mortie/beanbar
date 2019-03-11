#!/bin/sh
cd "$(dirname "$0")"
cat <<EOF
<!DOCTYPE html>
<html>
	<head>
		<meta charset="utf-8">
		<title>WebBar</title>
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
		</script>
		<script src="config:"></script>
	</body>
</html>
EOF
