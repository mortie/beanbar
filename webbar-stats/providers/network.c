#include "providers.h"

#include <dbus/dbus.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "src/ipc.h"

enum states {
	STATE_NONE,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_DISCONNECTING,
	STATE_DISCONNECTED,
};

static char *states[] = {
	[STATE_NONE] = "NONE",
	[STATE_CONNECTING] = "CONNECTING",
	[STATE_CONNECTED] = "CONNECTED",
	[STATE_DISCONNECTING] = "DISCONNECTING",
	[STATE_DISCONNECTED] = "DISCONNECTED",
};

size_t states_len = sizeof(states) / sizeof(*states);

static DBusMessage *prop_get(
		DBusConnection *conn, const char *target, const char *object,
		const char *iface, const char *prop) {

	DBusMessage *msg = dbus_message_new_method_call(
		target, object, "org.freedesktop.DBus.Properties", "Get");

	dbus_message_append_args(msg,
		DBUS_TYPE_STRING, &iface,
		DBUS_TYPE_STRING, &prop,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);
	DBusMessage *reply = dbus_connection_send_with_reply_and_block(
		conn, msg, 1000, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "property get error: %s\n", err.message);
		dbus_error_free(&err);
		return NULL;
	}
	dbus_error_free(&err);

	return reply;
}

static char *prop_get_string(
		DBusConnection *conn, const char *target, const char *object,
		const char *iface, const char *prop) {

	DBusMessage *reply = prop_get(conn, target, object, iface, prop);
	if (reply == NULL)
		return NULL;

	DBusMessageIter args;
	dbus_message_iter_init(reply, &args);

	DBusMessageIter sub;
	dbus_message_iter_recurse(&args, &sub);
	char *str;
	dbus_message_iter_get_basic(&sub, &str);
	dbus_message_unref(reply);
	return str;
}

static void send_active(DBusConnection *conn) {
	DBusMessage *reply = prop_get(
		conn, "org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager",
		"org.freedesktop.NetworkManager", "ActiveConnections");
	if (reply == NULL)
		return;

	DBusMessageIter args;
	dbus_message_iter_init(reply, &args);

	DBusMessageIter sub;
	dbus_message_iter_recurse(&args, &sub);
	DBusMessageIter subsub;
	dbus_message_iter_recurse(&sub, &subsub);

	do {
		char *path;
		dbus_message_iter_get_basic(&subsub, &path);
		char *name = prop_get_string(
			conn, "org.freedesktop.NetworkManager", path,
			"org.freedesktop.NetworkManager.Connection.Active", "Id");
		ipc_sendf("%s:%s:%s", path, states[STATE_CONNECTED], name);
	} while (dbus_message_iter_next(&subsub));

	dbus_message_unref(reply);
	return;
}

static void handle_state_changed(DBusConnection *conn, DBusMessage *msg) {
	DBusMessageIter args;
	dbus_message_iter_init(msg, &args);

	int argtype = dbus_message_iter_get_arg_type(&args);
	if (argtype != DBUS_TYPE_UINT32) {
		fprintf(stderr, "Unknown message type: %i (%c)\n", argtype, argtype);
		return;
	}

	uint32_t val;
	dbus_message_iter_get_basic(&args, &val);
	if (val >= states_len) {
		fprintf(stderr, "Invalid state: %i\n", val);
		return;
	}

	if (val == STATE_DISCONNECTED) {
		ipc_sendf("%s:%s:", dbus_message_get_path(msg), states[val]);
		return;
	}

	char *name = prop_get_string(
		conn, "org.freedesktop.NetworkManager", dbus_message_get_path(msg),
		"org.freedesktop.NetworkManager.Connection.Active", "Id");

	ipc_sendf("%s:%s:%s", dbus_message_get_path(msg), states[val], name);
}

static DBusHandlerResult filter (
		DBusConnection *conn, DBusMessage *msg, void *data) {

	if (strcmp(dbus_message_get_member(msg), "StateChanged") == 0)
		handle_state_changed(conn, msg);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static int main(int argc, char **argv) {
	DBusError err;
	DBusConnection* conn;

	dbus_error_init(&err);

	conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "DBus connection error: %s\n", err.message);
		dbus_error_init(&err);
		return EXIT_FAILURE;
	}

	const char *rule = "type='signal',interface='org.freedesktop.NetworkManager.Connection.Active'";
	dbus_bus_add_match(conn, rule, &err);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "DBus add match error: %s\n", err.message);
		dbus_error_init(&err);
		return EXIT_FAILURE;
	}

	// First send the initial state
	send_active(conn);

	dbus_error_free(&err);
	dbus_connection_add_filter(conn, filter, NULL, NULL);
	while (dbus_connection_read_write_dispatch(conn, -1));

	dbus_connection_close(conn);
	return EXIT_SUCCESS;
}

struct prov prov_wireless = { "network", main };
