#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "xdpw.h"

static const char object_path[] = "/org/freedesktop/portal/desktop";
static const char interface_name[] = "org.freedesktop.impl.portal.ScreenCast";

#define SDB_CHECK(x)  do { ret = x; if(ret < 0) { printf("Line %d: error %d\n", __LINE__, ret); return ret; }} while(0)

static int method_screencast_create_session(sd_bus_message *msg, void *data,
		sd_bus_error *ret_error) {
	int ret = 0;

	printf("=== CREATE SESSION\n");

	char *request_handle, *session_handle, *app_id;
	SDB_CHECK(sd_bus_message_read(msg, "oos", &request_handle, &session_handle, &app_id));
	SDB_CHECK(sd_bus_message_enter_container(msg, 'a', "{sv}"));

	printf("request_handle: %s\n", request_handle);
	printf("session_handle: %s\n", session_handle);
	printf("app_id: %s\n", app_id);

	char* key;
	while((ret = sd_bus_message_enter_container(msg, 'e', "sv")) > 0) {
		SDB_CHECK(sd_bus_message_read(msg, "s", &key));

		if(strcmp(key, "session_handle_token") == 0) {
			char* token;
			sd_bus_message_read(msg, "v", "s", &token);
			printf("Option token = %s\n", token);
		} else {
			printf("Unknown option %s\n", key);
			sd_bus_message_skip(msg, "v");
		}

		SDB_CHECK(sd_bus_message_exit_container(msg));
	}
	SDB_CHECK(ret);
	SDB_CHECK(sd_bus_message_exit_container(msg));

	// TODO: cleanup this
	struct xdpw_request *req =
		request_create(sd_bus_message_get_bus(msg), request_handle);
	if (req == NULL) {
		return -ENOMEM;
	}

	// TODO: cleanup this
	struct xdpw_session *sess =
		session_create(sd_bus_message_get_bus(msg), session_handle);
	if (sess == NULL) {
		return -ENOMEM;
	}

	sd_bus_message *reply = NULL;
	SDB_CHECK(sd_bus_message_new_method_return(msg, &reply));
	SDB_CHECK(sd_bus_message_append(reply, "ua{sv}", PORTAL_RESPONSE_SUCCESS, 0));
	SDB_CHECK(sd_bus_send(NULL, reply, NULL));
	sd_bus_message_unref(reply);
	return 0;
}


static int method_screencast_select_sources(sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
	int ret = 0;

	printf("=== SELECT SOURCES\n");

	char *request_handle, *session_handle, *app_id;
	SDB_CHECK(sd_bus_message_read(msg, "oos", &request_handle, &session_handle, &app_id));
	SDB_CHECK(sd_bus_message_enter_container(msg, 'a', "{sv}"));

	printf("request_handle: %s\n", request_handle);
	printf("session_handle: %s\n", session_handle);
	printf("app_id: %s\n", app_id);
	
	char* key;
	while((ret = sd_bus_message_enter_container(msg, 'e', "sv")) > 0) {
		SDB_CHECK(sd_bus_message_read(msg, "s", &key));

		if(strcmp(key, "multiple") == 0) {
			bool multiple;
			sd_bus_message_read(msg, "v", "b", &multiple);
			printf("Option multiple, val %x\n", multiple);
		} else if(strcmp(key, "types") == 0) {
			uint32_t mask;
			sd_bus_message_read(msg, "v", "u", &mask);
			printf("Option types, mask %x\n", mask);
		} else {
			printf("Unknown option %s\n", key);
			sd_bus_message_skip(msg, "v");
		}

		SDB_CHECK(sd_bus_message_exit_container(msg));
	}
	SDB_CHECK(ret);
	SDB_CHECK(sd_bus_message_exit_container(msg));


	sd_bus_message *reply = NULL;
	SDB_CHECK(sd_bus_message_new_method_return(msg, &reply));
	SDB_CHECK(sd_bus_message_append(reply, "ua{sv}", PORTAL_RESPONSE_SUCCESS, 0));
	SDB_CHECK(sd_bus_send(NULL, reply, NULL));
	sd_bus_message_unref(reply);
	return 0;
}

static int method_screencast_start(sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
	int ret = 0;
	
	printf("=== START\n");

	char *request_handle, *session_handle, *app_id, *parent_window;
	SDB_CHECK(sd_bus_message_read(msg, "ooss", &request_handle, &session_handle, &app_id, &parent_window));
	SDB_CHECK(sd_bus_message_enter_container(msg, 'a', "{sv}"));

	printf("request_handle: %s\n", request_handle);
	printf("session_handle: %s\n", session_handle);
	printf("app_id: %s\n", app_id);
	printf("parent_window: %s\n", parent_window);
	
	char* key;
	while((ret = sd_bus_message_enter_container(msg, 'e', "sv")) > 0) {
		SDB_CHECK(sd_bus_message_read(msg, "s", &key));

		printf("Unknown option %s\n", key);
		sd_bus_message_skip(msg, "v");

		SDB_CHECK(sd_bus_message_exit_container(msg));
	}
	SDB_CHECK(ret);
	SDB_CHECK(sd_bus_message_exit_container(msg));

	sd_bus_message *reply = NULL;
	SDB_CHECK(sd_bus_message_new_method_return(msg, &reply));
	SDB_CHECK(sd_bus_message_append(reply, "ua{sv}", PORTAL_RESPONSE_SUCCESS, 0));
	SDB_CHECK(sd_bus_send(NULL, reply, NULL));
	sd_bus_message_unref(reply);
	return 0;
}

static const sd_bus_vtable screencast_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("CreateSession", "oosa{sv}", "ua{sv}", method_screencast_create_session, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("SelectSources", "oosa{sv}", "ua{sv}", method_screencast_select_sources, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("Start", "oossa{sv}", "ua{sv}", method_screencast_start, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_VTABLE_END
};

int init_screencast(sd_bus *bus) {
	// TODO: cleanup
	sd_bus_slot *slot = NULL;
	return sd_bus_add_object_vtable(bus, &slot, object_path, interface_name,
		screencast_vtable, NULL);
}
