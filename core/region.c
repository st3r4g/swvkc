#define _POSIX_C_SOURCE 200809L
#include <core/region.h>
#include <wayland-server-protocol.h>

static void region_destroy(struct wl_client *client, struct wl_resource
*resource) {

}

static void region_add(struct wl_client *client, struct wl_resource *resource,
int32_t x, int32_t y, int32_t width, int32_t height) {

}

static void region_subtract(struct wl_client *client, struct wl_resource
*resource, int32_t x, int32_t y, int32_t width, int32_t height) {

}

static const struct wl_region_interface impl = {
	.destroy = region_destroy,
	.add = region_add,
	.subtract = region_subtract
};

void region_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
