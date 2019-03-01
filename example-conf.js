ipcSend("exec", `
	${IPC_EXEC_C}
	#include <stdio.h>
	#include <unistd.h>
	int main() {
		while (1) {
			printf("Hello World\\n");
			fflush(stdout);
			sleep(2);
		}
	}
`, function(msg) {
	console.log("thing 2 got:", msg);
});

init(h(Label, { text: "Hello World" }));
