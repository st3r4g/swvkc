#define _POSIX_C_SOURCE 200809L
#include <wayland-server-core.h>

#include <core/wl_surface.h>
#include <extensions/linux-explicit-synchronization-v1/zwp_linux_surface_synchronization_v1.h>
#include <util/log.h>
#include <util/util.h>

#include <linux-explicit-synchronization-unstable-v1-server-protocol.h>


static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void get_synchronization(struct wl_client *client, struct wl_resource
*resource, uint32_t id, struct wl_resource *surface_resource) {
	struct surface *surface = wl_resource_get_user_data(surface_resource);
	if (util_get_extension(&surface->extensions,
	 "zwp_linux_surface_synchronization_v1"))
		wl_resource_post_error(resource,
		 ZWP_LINUX_EXPLICIT_SYNCHRONIZATION_V1_ERROR_SYNCHRONIZATION_EXISTS,
		  "the surface already has a synchronization object associated");

	struct wl_resource *child = wl_resource_create(client,
	&zwp_linux_surface_synchronization_v1_interface, 1, id);
	linux_surface_synchronization_new(child, surface);
}

static const struct zwp_linux_explicit_synchronization_v1_interface impl = {
	.destroy = destroy,
	.get_synchronization = get_synchronization
};

void linux_explicit_synchronization_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
