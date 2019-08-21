#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>

#include <wayland-server-protocol.h> // Just for one event
#include <xdg-shell-server-protocol.h>
#include <core/wl_surface.h>
#include <core/wl_surface_staged_field.h>
#include <extensions/xdg_shell/xdg_surface.h>
#include <util/log.h>
#include <util/util.h>

/*static enum wl_iterator_result keyboard_set(struct wl_resource *resource,
void *user_data) {
	if (!strcmp(wl_resource_get_class(resource), "wl_keyboard")) {
		struct xdg_surface0 *xdg_surface = user_data;
		xdg_surface->keyboard = resource;
		return WL_ITERATOR_STOP;
	}
	return WL_ITERATOR_CONTINUE;
}*/

bool xdg_surface_is_mapped(struct xdg_surface0 *xdg_surface) {
	static const uint8_t all_conditions_set =
	 MAP_CONDITION_ROLE_ASSIGNED |
	 MAP_CONDITION_BASE_ROLE_INIT_COMMIT |
	 MAP_CONDITION_ROLE_INIT_COMMIT |
	 MAP_CONDITION_BUFFER_COMMIT;
	return xdg_surface->map_conditions == all_conditions_set;
}

bool xdg_surface_map_condition_check(struct xdg_surface0 *xdg_surface, enum
map_condition_field condition) {
	return xdg_surface->map_conditions & condition;
}

void xdg_surface_map_condition_satisfied(struct xdg_surface0 *xdg_surface, enum
map_condition_field condition) {
	xdg_surface->map_conditions |= condition;
	if (xdg_surface_is_mapped(xdg_surface)) {
		struct surface *surface = wl_resource_get_user_data(xdg_surface->surface);
		void *user_data = surface->surface_events.user_data;
		surface->surface_events.map(surface, user_data);
		surface->is_mapped = true;
	}
}

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void get_toplevel(struct wl_client *client, struct wl_resource *resource,
uint32_t id) {
	struct xdg_surface0 *data = wl_resource_get_user_data(resource);
	struct wl_resource *toplevel_resource = wl_resource_create(client,
	&xdg_toplevel_interface, 1, id);
	struct xdg_toplevel_data *xdg_toplevel =
	 xdg_toplevel_new(toplevel_resource, data, data->xdg_toplevel_events);
/*
 * Assign a role to the surface
 */
	struct surface *surface = wl_resource_get_user_data(data->surface);
	bool role_check = surface->role == ROLE_NONE      ||
	                  surface->role == ROLE_XDG_TOPLEVEL;
	bool base_role_check = surface->base_role == BASE_ROLE_XDG_SURFACE;
	if (!(role_check && base_role_check)) {
		wl_resource_post_error(data->wm_base, XDG_WM_BASE_ERROR_ROLE,
		 "Given wl_surface has another role");
		return;
	}
	surface->role = ROLE_XDG_TOPLEVEL;
	surface->role_object = xdg_toplevel;
	xdg_surface_map_condition_satisfied(data,
	 MAP_CONDITION_ROLE_ASSIGNED);
}

static void get_popup(struct wl_client *client, struct wl_resource *resource,
uint32_t id, struct wl_resource *parent, struct wl_resource *positioner) {

}

static void set_window_geometry(struct wl_client *client, struct wl_resource
*resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	struct xdg_surface0 *xdg_surface = wl_resource_get_user_data(resource);
	xdg_surface->pending->window_geometry.x = x;
	xdg_surface->pending->window_geometry.y = y;
	xdg_surface->pending->window_geometry.width = width;
	xdg_surface->pending->window_geometry.height = height;

}

static void ack_configure(struct wl_client *client, struct wl_resource
*resource, uint32_t serial) {
	struct xdg_surface0 *xdg_surface = wl_resource_get_user_data(resource);
	struct wl_array array;
	wl_array_init(&array); //Need the currently pressed keys
	xdg_surface->keyboard = util_wl_client_get_keyboard(client);
//	wl_client_for_each_resource(client, keyboard_set, xdg_surface);
	if (xdg_surface->keyboard) //TODO: not sure this is the right place
	wl_keyboard_send_enter(xdg_surface->keyboard, 0, xdg_surface->surface,
	&array);
}

static const struct xdg_surface_interface impl = {
	.destroy = destroy,
	.get_toplevel = get_toplevel,
	.get_popup = get_popup,
	.set_window_geometry = set_window_geometry,
	.ack_configure = ack_configure
};

static void commit_notify(struct wl_listener *listener, void *data) {
	struct xdg_surface0 *xdg_surface;
	xdg_surface = wl_container_of(listener, xdg_surface, commit);
	struct xdg_surface_state0 *pending = xdg_surface->pending, *current =
	xdg_surface->current;

	current->window_geometry.x = pending->window_geometry.x;
	current->window_geometry.y = pending->window_geometry.y;
	current->window_geometry.width = pending->window_geometry.width;
	current->window_geometry.height = pending->window_geometry.height;
	
	if (!xdg_surface_map_condition_check(xdg_surface,
	                                MAP_CONDITION_BASE_ROLE_INIT_COMMIT)) {
		xdg_surface_map_condition_satisfied(xdg_surface,
		                          MAP_CONDITION_BASE_ROLE_INIT_COMMIT);
	}
}

static void xdg_surface_free(struct wl_resource *resource) {
	errlog("xdg_surface destroyed");
	struct xdg_surface0 *xdg_surface = wl_resource_get_user_data(resource);
	xdg_surface->child_destroy_notify(xdg_surface->data);
	free(xdg_surface->current);
	free(xdg_surface->pending);
	free(xdg_surface);
}

struct xdg_surface0 *xdg_surface_new(struct wl_resource *resource, struct
wl_resource *surface_resource, struct wl_resource *wm_base, struct
xdg_toplevel_events xdg_toplevel_events, callback_t child_destroy_notify, void
*data) {
	struct surface *surface = wl_resource_get_user_data(surface_resource);
	struct xdg_surface0 *xdg_surface = calloc(1, sizeof(struct
	xdg_surface0));
	xdg_surface->self = resource;
	xdg_surface->wm_base = wm_base;
	xdg_surface->pending = calloc(1, sizeof(struct xdg_surface_state0));
	xdg_surface->current = calloc(1, sizeof(struct xdg_surface_state0));
	xdg_surface->surface = surface_resource;

	xdg_surface->xdg_toplevel_events = xdg_toplevel_events;

	xdg_surface->child_destroy_notify = child_destroy_notify;
	xdg_surface->data = data;

	xdg_surface->commit.notify = commit_notify;
	wl_signal_add(&surface->commit, &xdg_surface->commit);
	wl_resource_set_implementation(resource, &impl, xdg_surface,
	xdg_surface_free);
	return xdg_surface;
}
