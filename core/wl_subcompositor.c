#define _POSIX_C_SOURCE 200809L

#include <wayland-server-protocol.h>

#include <core/wl_subsurface.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {

}

static void get_subsurface(struct wl_client *client, struct wl_resource
*resource, uint32_t id, struct wl_resource *surface, struct wl_resource
*parent) {
	struct wl_resource *subsurface_resource = wl_resource_create(client,
	&wl_subsurface_interface, 1, id);
	wl_subsurface_new(subsurface_resource);
}

static const struct wl_subcompositor_interface impl = {
	.destroy = destroy,
	.get_subsurface = get_subsurface
};

void wl_subcompositor_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
