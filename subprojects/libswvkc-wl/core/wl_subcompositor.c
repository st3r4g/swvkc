#define _POSIX_C_SOURCE 200809L

#include <wayland-server-protocol.h>

#include <core/subsurface.h>
#include <core/wl_surface.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void get_subsurface(struct wl_client *client, struct wl_resource
*resource, uint32_t id, struct wl_resource *surface_resource, struct wl_resource
*parent) {
	struct surface *surface = wl_resource_get_user_data(surface_resource);
	bool role_check = surface->role == ROLE_NONE ||
	                  surface->role == ROLE_SUBSURFACE;
	if (!role_check) {
		wl_resource_post_error(resource, WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
		 "the to-be sub-surface must not already have another role");
		return;
	} else if (surface->role_object) {
		wl_resource_post_error(resource, WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
		 "the to-be sub-surface must not have an existing wl_subsurface object");
		return;
	}

	struct wl_resource *subsurface_resource = wl_resource_create(client,
	&wl_subsurface_interface, 1, id);
	struct subsurface *subsurface = wl_subsurface_new(subsurface_resource,
	surface_resource, parent);
	
	surface->role = ROLE_SUBSURFACE;
	surface->role_object = subsurface;
}

static const struct wl_subcompositor_interface impl = {
	.destroy = destroy,
	.get_subsurface = get_subsurface
};

void wl_subcompositor_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
