#define _POSIX_C_SOURCE 200809L

#include <wayland-server-protocol.h>

#include <core/wl_surface.h>
#include <core/wl_pointer.h>
#include <util/log.h>

#include <stdlib.h>

static void set_cursor(struct wl_client *client, struct wl_resource *resource,
uint32_t serial, struct wl_resource *surface_resource, int32_t hotspot_x,
int32_t hotspot_y) {
	if (surface_resource == NULL)
		return;
	struct pointer *pointer = wl_resource_get_user_data(resource);
	pointer->hotspot_x = hotspot_x;
	pointer->hotspot_y = hotspot_y;
/*
 * Assign a role to the surface
 */
	struct surface *surface = wl_resource_get_user_data(surface_resource);
	bool role_check = surface->role == ROLE_NONE      ||
	                  surface->role == ROLE_CURSOR;
	bool base_role_check = surface->base_role == BASE_ROLE_NONE;
	if (!(role_check && base_role_check)) {
		wl_resource_post_error(resource, WL_POINTER_ERROR_ROLE,
		 "Given wl_surface has another role");
		return;
	}
	surface->role = ROLE_CURSOR;
	surface->role_object = pointer;
	if (!surface->is_mapped) {
		void *user_data = surface->surface_events.user_data;
		surface->surface_events.map(surface, user_data);
		surface->is_mapped = true;
	}
}

static void release(struct wl_client *client, struct wl_resource *resource) {

}

static const struct wl_pointer_interface impl = {
	.set_cursor = set_cursor,
	.release = release
};

void wl_pointer_new(struct wl_resource *resource) {
	struct pointer *pointer = calloc(1, sizeof(struct pointer));
	wl_resource_set_implementation(resource, &impl, pointer, 0);
}
