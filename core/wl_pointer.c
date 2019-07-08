#define _POSIX_C_SOURCE 200809L

#include <wayland-server-protocol.h>

static void set_cursor(struct wl_client *client, struct wl_resource *resource,
uint32_t serial, struct wl_resource *surface, int32_t hotspot_x, int32_t
hotspot_y) {

}

static void release(struct wl_client *client, struct wl_resource *resource) {

}

static const struct wl_pointer_interface impl = {
	.set_cursor = set_cursor,
	.release = release
};

void wl_pointer_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
