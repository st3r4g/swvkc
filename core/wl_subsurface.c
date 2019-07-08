#define _POSIX_C_SOURCE 200809L

#include <wayland-server-protocol.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {

}

static void set_position(struct wl_client *client, struct wl_resource *resource,
int32_t x, int32_t y) {

}

static void place_above(struct wl_client *client, struct wl_resource *resource,
struct wl_resource *sibling) {

}

static void place_below(struct wl_client *client, struct wl_resource *resource,
struct wl_resource *sibling) {

}

static void set_sync(struct wl_client *client, struct wl_resource *resource) {

}

static void set_desync(struct wl_client *client, struct wl_resource *resource) {

}

static const struct wl_subsurface_interface impl = {
	.destroy = destroy,
	.set_position = set_position,
	.place_above = place_above,
	.place_below = place_below,
	.set_sync = set_sync,
	.set_desync = set_desync,
};

void wl_subsurface_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
