#define _POSIX_C_SOURCE 200809L
#include <util/log.h>
#include <core/seat.h>
#include <wayland-server-protocol.h>
#include <stdlib.h>

#include <core/wl_pointer.h>

static void get_pointer(struct wl_client *client, struct wl_resource *resource,
uint32_t id) {
	struct wl_resource *pointer_resource = wl_resource_create(client,
	&wl_pointer_interface, wl_resource_get_version(resource), id);
	wl_pointer_new(pointer_resource);
}

static void seat_release(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void seat_get_keyboard(struct wl_client *client, struct wl_resource
*resource, uint32_t id) {
	struct seat *seat = wl_resource_get_user_data(resource);
	struct wl_resource *keyboard_resource = wl_resource_create(client,
	&wl_keyboard_interface, wl_resource_get_version(resource), id);
	keyboard_new(keyboard_resource, seat->keyboard_events);
}

static const struct wl_seat_interface impl = {
	.get_pointer = get_pointer,
	.get_keyboard = seat_get_keyboard,
	.get_touch = 0, // should post a protocol error instead
	.release = seat_release
};

void seat_free(struct wl_resource *resource) {
	struct seat *seat = wl_resource_get_user_data(resource);
	free(seat);
}

struct seat *seat_new(struct wl_resource *resource, struct
keyboard_events keyboard_events) {
	struct seat *seat = calloc(1, sizeof(struct seat));
	seat->keyboard_events = keyboard_events;

	wl_resource_set_implementation(resource, &impl, seat, seat_free);

	if (wl_resource_get_version(resource) >= WL_SEAT_CAPABILITIES_SINCE_VERSION)
		wl_seat_send_capabilities(resource, WL_SEAT_CAPABILITY_KEYBOARD
		 | WL_SEAT_CAPABILITY_POINTER);
	if (wl_resource_get_version(resource) >= WL_SEAT_NAME_SINCE_VERSION)
		wl_seat_send_name(resource, "seat0");

	return seat;
}
