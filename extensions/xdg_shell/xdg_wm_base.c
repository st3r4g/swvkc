#define _POSIX_C_SOURCE 200809L

#include <xdg-shell-server-protocol.h>
#include <wayland-server-protocol.h> //TODO: remove
#include <extensions/xdg_shell/xdg_wm_base.h>
#include <extensions/xdg_shell/xdg_surface.h>
#include <server.h>
#include <stdlib.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {

}

static void create_positioner(struct wl_client *client, struct wl_resource
*resource, uint32_t id) {

}

static void get_xdg_surface(struct wl_client *client, struct wl_resource
*resource, uint32_t id, struct wl_resource *surface) {
	struct xdg_wm_base *xdg_wm_base = wl_resource_get_user_data(resource);
	struct wl_resource *xdg_surf_resource = wl_resource_create(client,
	&xdg_surface_interface, 1, id);
	xdg_surface_new(xdg_surf_resource, surface, xdg_wm_base->server);
}

static void pong(struct wl_client *client, struct wl_resource *resource,
uint32_t serial) {

}

static const struct xdg_wm_base_interface impl = {
	.destroy = destroy,
	.create_positioner = create_positioner,
	.get_xdg_surface = get_xdg_surface,
	.pong = pong
};

struct xdg_wm_base *xdg_wm_base_new(struct wl_resource *resource, struct server
*server) {
	struct xdg_wm_base *xdg_wm_base = calloc(1, sizeof(struct xdg_wm_base));
	xdg_wm_base->server = server;
	wl_list_init(&xdg_wm_base->xdg_toplevel_list); //remove
	wl_list_init(&xdg_wm_base->xdg_popup_list); //remove
	wl_resource_set_implementation(resource, &impl, xdg_wm_base, 0);
	return xdg_wm_base;
}
