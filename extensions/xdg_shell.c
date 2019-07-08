#define _POSIX_C_SOURCE 200809L
#include <core/wl_surface.h>
#include <extensions/xdg_shell.h>
#include <util/log.h>
#include <protocols/xdg-shell-unstable-v6-server-protocol.h>
#include <wayland-server-protocol.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* XDG TOPLEVEL */

static void xdg_toplevel_destroy(struct wl_client *client, struct wl_resource
*resource) {

}

static void xdg_toplevel_set_parent(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *parent) {

}

static void xdg_toplevel_set_title(struct wl_client *client, struct wl_resource
*resource, const char *title) {

}

static void xdg_toplevel_set_app_id(struct wl_client *client, struct wl_resource
*resource, const char *app_id) {

}

static void xdg_toplevel_show_window_menu(struct wl_client *client, struct
wl_resource *resource, struct wl_resource *seat, uint32_t serial, int32_t x,
int32_t y) {

}

static void xdg_toplevel_move(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *seat, uint32_t serial) {

}

static void xdg_toplevel_resize(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *seat, uint32_t serial, uint32_t edges) {

}

static void xdg_toplevel_set_max_size(struct wl_client *client, struct
wl_resource *resource, int32_t width, int32_t height) {

}

static void xdg_toplevel_set_min_size(struct wl_client *client, struct
wl_resource *resource, int32_t width, int32_t height) {

}

static void xdg_toplevel_set_maximized(struct wl_client *client, struct
wl_resource *resource) {

}

static void xdg_toplevel_unset_maximized(struct wl_client *client, struct
wl_resource *resource) {

}

static void xdg_toplevel_set_fullscreen(struct wl_client *client, struct
wl_resource *resource, struct wl_resource *output) {

}

static void xdg_toplevel_unset_fullscreen(struct wl_client *client, struct
wl_resource *resource) {

}

static void xdg_toplevel_set_minimized(struct wl_client *client, struct
wl_resource *resource) {

}

static const struct zxdg_toplevel_v6_interface toplevel_impl = {
	.destroy = xdg_toplevel_destroy,
	.set_parent = xdg_toplevel_set_parent,
	.set_title = xdg_toplevel_set_title,
	.set_app_id = xdg_toplevel_set_app_id,
	.show_window_menu = xdg_toplevel_show_window_menu,
	.move = xdg_toplevel_move,
	.resize = xdg_toplevel_resize,
	.set_max_size = xdg_toplevel_set_max_size,
	.set_min_size = xdg_toplevel_set_min_size,
	.set_maximized = xdg_toplevel_set_maximized,
	.unset_maximized = xdg_toplevel_unset_maximized,
	.set_fullscreen = xdg_toplevel_set_fullscreen,
	.unset_fullscreen = xdg_toplevel_unset_fullscreen,
	.set_minimized = xdg_toplevel_set_minimized
};

/* XDG SURFACE */

static void xdg_surface_destroy(struct wl_client *client, struct wl_resource
*resource) {
	errlog("xdg_surface_destroy");
//	wl_resource_destroy(resource);
	errlog("gdshgd");
}

static void xdg_surface_get_toplevel(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {
	struct wl_resource *toplevel_resource = wl_resource_create(client,
	&zxdg_toplevel_v6_interface, 1, id);
	wl_resource_set_implementation(toplevel_resource, &toplevel_impl, 0, 0);
	struct wl_array array;
	wl_array_init(&array);
	int32_t *state1 = wl_array_add(&array, sizeof(int32_t));
	*state1 = ZXDG_TOPLEVEL_V6_STATE_ACTIVATED;
	int32_t *state2 = wl_array_add(&array, sizeof(int32_t));
	*state2 = ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED;
	zxdg_toplevel_v6_send_configure(toplevel_resource, 0, 0, &array);
	zxdg_surface_v6_send_configure(resource, 0);
}

static void xdg_surface_get_popup(struct wl_client *client, struct wl_resource
*resource, uint32_t id, struct wl_resource *parent, struct wl_resource
*positioner) {

}

static void xdg_surface_set_window_geometry(struct wl_client *client, struct
wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	errlog("window geometry on %p: %i %i %i %i", (void*)resource, x, y, width, height);
	struct xdg_surface *xdg_surface = wl_resource_get_user_data(resource);
	xdg_surface->pending->window_geometry.x = x;
	xdg_surface->pending->window_geometry.y = y;
	xdg_surface->pending->window_geometry.width = width;
	xdg_surface->pending->window_geometry.height = height;
}

static enum wl_iterator_result keyboard_set(struct wl_resource *resource,
void *user_data) {
	if (!strcmp(wl_resource_get_class(resource), "wl_keyboard")) {
		struct xdg_surface *xdg_surface = user_data;
		xdg_surface->keyboard = resource;
		return WL_ITERATOR_STOP;
	}
	return WL_ITERATOR_CONTINUE;
}

static void xdg_surface_ack_configure(struct wl_client *client, struct
wl_resource *resource, uint32_t serial) {
	struct xdg_surface *xdg_surface = wl_resource_get_user_data(resource);
	wl_client_for_each_resource(client, keyboard_set, xdg_surface);
	if (xdg_surface->keyboard)
		server_focus(xdg_surface);
}

static const struct zxdg_surface_v6_interface surface_impl = {
	.destroy = xdg_surface_destroy,
	.get_toplevel = xdg_surface_get_toplevel,
	.get_popup = xdg_surface_get_popup,
	.set_window_geometry = xdg_surface_set_window_geometry,
	.ack_configure = xdg_surface_ack_configure
};

static void commit_notify(struct wl_listener *listener, void *data) {
	struct xdg_surface *xdg_surface;
	xdg_surface = wl_container_of(listener, xdg_surface, commit);
	struct xdg_surface_state *pending = xdg_surface->pending, *current =
	xdg_surface->current;

	current->window_geometry.x = pending->window_geometry.x;
	current->window_geometry.y = pending->window_geometry.y;
	current->window_geometry.width = pending->window_geometry.width;
	current->window_geometry.height = pending->window_geometry.height;
}

static void xdg_surface_free(struct wl_resource *resource) {
	struct xdg_surface *xdg_surface = wl_resource_get_user_data(resource);
	wl_list_remove(&xdg_surface->link);
	free(xdg_surface->current);
	free(xdg_surface->pending);
	free(xdg_surface);
}

struct xdg_surface *xdg_surface_v6_new(struct wl_resource *resource, struct
wl_resource *surface_resource, struct server *server) {
	struct surface *surface = wl_resource_get_user_data(surface_resource);
	struct xdg_surface *xdg_surface = calloc(1, sizeof(struct
	xdg_surface));
	xdg_surface->pending = calloc(1, sizeof(struct xdg_surface_state));
	xdg_surface->current = calloc(1, sizeof(struct xdg_surface_state));
	xdg_surface->surface = surface_resource;
	xdg_surface->commit.notify = commit_notify;
	wl_signal_add(&surface->commit, &xdg_surface->commit);
	xdg_surface->server = server;
	wl_resource_set_implementation(resource, &surface_impl,
	xdg_surface, xdg_surface_free);
	return xdg_surface;
}

/* XDG SHELL */

static void xdg_shell_destroy(struct wl_client *client, struct wl_resource
*resource) {

}

static void xdg_shell_create_positioner(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {

}

static void xdg_shell_get_xdg_surface(struct wl_client *client, struct
wl_resource *resource, uint32_t id, struct wl_resource *surface) {
	struct xdg_shell *xdg_shell = wl_resource_get_user_data(resource);
	struct wl_resource *xdg_surf_resource = wl_resource_create(client,
	&zxdg_surface_v6_interface, 1, id);
	struct xdg_surface *xdg_surface = xdg_surface_v6_new(xdg_surf_resource,
	surface, xdg_shell->server);
	wl_list_insert(&xdg_shell->xdg_surface_list, &xdg_surface->link);
}

static void xdg_shell_pong(struct wl_client *client, struct wl_resource
*resource, uint32_t serial) {

}

static const struct zxdg_shell_v6_interface impl = {
	.destroy = xdg_shell_destroy,
	.create_positioner = xdg_shell_create_positioner,
	.get_xdg_surface = xdg_shell_get_xdg_surface,
	.pong = xdg_shell_pong
};

struct xdg_shell *xdg_shell_new(struct wl_resource *resource, struct server
*server) {
	struct xdg_shell *xdg_shell = calloc(1, sizeof(struct xdg_shell));
	wl_list_init(&xdg_shell->xdg_surface_list);
	xdg_shell->server = server;
	wl_resource_set_implementation(resource, &impl, xdg_shell, 0);
	return xdg_shell;
}
