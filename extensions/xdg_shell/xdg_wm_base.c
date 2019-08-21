#define _POSIX_C_SOURCE 200809L

#include <core/wl_surface.h>
#include <extensions/xdg_shell/xdg_wm_base.h>

#include <wayland-server-core.h>
#include <xdg-shell-server-protocol.h>

#include <stdlib.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	struct xdg_wm_base *xdg_wm_base = wl_resource_get_user_data(resource);
	if (xdg_wm_base->xdg_surface_count != 0)
		wl_resource_post_error(resource,
		 XDG_WM_BASE_ERROR_DEFUNCT_SURFACES, "xdg_wm_base was destroyed \
		  before children");
	wl_resource_destroy(resource);
}

static void create_positioner(struct wl_client *client, struct wl_resource
*resource, uint32_t id) {

}

static void child_destroy_notify(void *user_data) {
	int *xdg_surface_count = user_data;
	(*xdg_surface_count)--;
}

static void get_xdg_surface(struct wl_client *client, struct wl_resource
*resource, uint32_t id, struct wl_resource *surface_resource) {
	struct xdg_wm_base *xdg_wm_base = wl_resource_get_user_data(resource);
	struct wl_resource *xdg_surf_resource = wl_resource_create(client,
	&xdg_surface_interface, 1, id);
	struct xdg_surface0 *xdg_surface = xdg_surface_new(xdg_surf_resource,
	surface_resource, resource, xdg_wm_base->xdg_toplevel_events,
	child_destroy_notify, &xdg_wm_base->xdg_surface_count);
/*
 * Assign a role to the surface
 */
	struct surface *surface = wl_resource_get_user_data(surface_resource);
	bool role_check = surface->role == ROLE_NONE      ||
	                  surface->role == ROLE_XDG_POPUP ||
	                  surface->role == ROLE_XDG_TOPLEVEL;
	bool base_role_check = surface->base_role == BASE_ROLE_NONE ||
	                       surface->base_role == BASE_ROLE_XDG_SURFACE;
	if (!(role_check && base_role_check)) {
		wl_resource_post_error(resource, XDG_WM_BASE_ERROR_ROLE,
		 "Given wl_surface has another role");
		return;
	}
	surface->base_role = BASE_ROLE_XDG_SURFACE;
	surface->base_role_object = xdg_surface;
	xdg_wm_base->xdg_surface_count++;
}

static void pong(struct wl_client *client, struct wl_resource *resource,
uint32_t serial) {
	/*
	 * Never sending ping at the moment
	 */
}

static const struct xdg_wm_base_interface impl = {
	.destroy = destroy,
	.create_positioner = create_positioner,
	.get_xdg_surface = get_xdg_surface,
	.pong = pong
};

struct xdg_wm_base *xdg_wm_base_new(struct wl_resource *resource, struct server
*server, struct xdg_toplevel_events xdg_toplevel_events) {
	struct xdg_wm_base *xdg_wm_base = calloc(1, sizeof(struct xdg_wm_base));
	xdg_wm_base->server = server;
	xdg_wm_base->xdg_toplevel_events = xdg_toplevel_events;
	wl_resource_set_implementation(resource, &impl, xdg_wm_base, 0);
	return xdg_wm_base;
}
