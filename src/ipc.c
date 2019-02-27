#include "ipc.h"

#include <mkjson/mkjson.h>

#include "log.h"

static void send_msg_cb(GObject *source, GAsyncResult *res, gpointer data) {
	(void)source;
	(void)res;
	(void)data;
}

static void send_msg(struct ipc *ipc, int id, const char *msg) {
	char pre[] = "window.onIPCMessage(";
	size_t pre_len = strlen(pre);
	char post[] = ");";
	char *json = mkjson(MKJSON_OBJ, 2, MKJSON_INT, "id", id, MKJSON_STRING, "msg", msg);
	size_t json_len = strlen(json);

	char *js = malloc(strlen(pre) + strlen(json) + strlen(post) + 1);
	strcpy(js, pre);
	strcpy(js + pre_len, json);
	strcpy(js + pre_len + json_len, post);
	free(json);

	webkit_web_view_run_javascript(ipc->view, js, NULL, send_msg_cb, NULL);
	free(js);
}

static void on_msg(
		WebKitUserContentManager *manager, WebKitJavascriptResult *jsresult,
		gpointer data) {
	(void)manager;
	struct ipc *ipc = (struct ipc *)data;
	(void)ipc;
	JSCValue *val = webkit_javascript_result_get_js_value(jsresult);

	if (!jsc_value_is_object(val)) {
		warn("IPC expected object.");
		return;
	}

	JSCValue *valid = jsc_value_object_get_property(val, "id");
	if (valid == NULL) {
		warn("IPC expected object with an `id` property.");
		return;
	}

	JSCValue *valmsg = jsc_value_object_get_property(val, "msg");
	if (valmsg == NULL) {
		warn("IPC expected object with a `msg` property.");
		return;
	}

	JSCValue *valtype = jsc_value_object_get_property(val, "type");
	if (valtype == NULL) {
		warn("IPC expected object with a `type` property.");
		return;
	}

	debug("type is %s", jsc_value_to_string(valtype));
	send_msg(ipc, 10, "hello world");
}

void ipc_init(struct ipc *ipc, WebKitWebView *view) {
	ipc->view = view;
	WebKitUserContentManager *webmgr = webkit_web_view_get_user_content_manager(view);
	g_signal_connect(webmgr, "script-message-received::ipc",
		G_CALLBACK(on_msg), ipc);
	webkit_user_content_manager_register_script_message_handler(webmgr, "ipc");
}
