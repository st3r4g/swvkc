#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xdg-shell-server-protocol.h>
#include <extensions/xdg_shell/xdg_toplevel.h>
#include <extensions/xdg_shell/xdg_surface.h>
#include <core/wl_surface.h>
#include <backend/screen.h>
#include <server.h>
#include <util/log.h>
#include <util/util.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {

}

static void set_parent(struct wl_client *client, struct wl_resource *resource,
struct wl_resource *parent) {

}

static void set_title(struct wl_client *client, struct wl_resource *resource,
const char *title) {

}

static void set_app_id(struct wl_client *client, struct wl_resource *resource,
const char *app_id) {
	struct xdg_toplevel_data *data = wl_resource_get_user_data(resource);
	data->app_id = strdup(app_id);
}

static void show_window_menu(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *seat, uint32_t serial, int32_t x, int32_t y) {

}

static void move(struct wl_client *client, struct wl_resource *resource, struct
wl_resource *seat, uint32_t serial) {

}

static void resize(struct wl_client *client, struct wl_resource *resource,
struct wl_resource *seat, uint32_t serial, uint32_t edges) {

}

static void set_max_size(struct wl_client *client, struct wl_resource *resource,
int32_t width, int32_t height) {

}

static void set_min_size(struct wl_client *client, struct wl_resource *resource,
int32_t width, int32_t height) {

}

static void set_maximized(struct wl_client *client, struct wl_resource
*resource) {

}

static void unset_maximized(struct wl_client *client, struct wl_resource
*resource) {

}

static void set_fullscreen(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *output) {

}

static void unset_fullscreen(struct wl_client *client, struct wl_resource
*resource) {

}

static void set_minimized(struct wl_client *client, struct wl_resource
*resource) {

}

static const struct xdg_toplevel_interface impl = {
	.destroy = destroy,
	.set_parent = set_parent,
	.set_title = set_title,
	.set_app_id = set_app_id,
	.show_window_menu = show_window_menu,
	.move = move,
	.resize = resize,
	.set_max_size = set_max_size,
	.set_min_size = set_min_size,
	.set_maximized = set_maximized,
	.unset_maximized = unset_maximized,
	.set_fullscreen = set_fullscreen,
	.unset_fullscreen = unset_fullscreen,
	.set_minimized = set_minimized,
};

static void commit_notify(struct wl_listener *listener, void *data) {
	struct surface *surface = data;
	if (!surface->is_mapped && surface->current->buffer) {
		surface->surface_events.map(surface,
		 surface->surface_events.user_data);
		surface->is_mapped = true;
		errlog("%d", surface->is_mapped);
		errlog("%p", surface);
	}
}

static void destroyed(struct wl_resource *resource) {
	errlog("xdg_toplevel destroyed");
	struct xdg_toplevel_data *data = wl_resource_get_user_data(resource);
/*	struct surface *surface = wl_resource_get_user_data(data->xdg_surface_data->surface);
	errlog("%p", surface);
	if (surface->is_mapped) {
		surface->surface_events.unmap(surface,
		 surface->surface_events.user_data);
		surface->is_mapped = false;
	}*/
	server_window_destroy(data);
	errlog("Destroyed the window '%s'", data->app_id); //TODO move to main
	free(data->app_id);
	free(data);
}

// version 1

void xdg_toplevel_new(struct wl_resource *resource, struct xdg_surface0
*xdg_surface_data, struct server *server) {
	struct xdg_toplevel_data *toplevel_data = calloc(1, sizeof(struct
	xdg_toplevel_data));
	toplevel_data->server = server;
	toplevel_data->xdg_surface_data = xdg_surface_data;
	struct surface *surface_data =
	wl_resource_get_user_data(xdg_surface_data->surface);
	errlog("%p", surface_data);
	toplevel_data->commit.notify = commit_notify;
	wl_signal_add(&surface_data->commit, &toplevel_data->commit);
	toplevel_data->app_id = get_a_name(wl_resource_get_client(resource));
	wl_resource_set_implementation(resource, &impl, toplevel_data,
	destroyed);
	server_window_create(toplevel_data);
	struct wl_array array;
	wl_array_init(&array);
	int32_t *state1 = wl_array_add(&array, sizeof(int32_t));
	*state1 = XDG_TOPLEVEL_STATE_ACTIVATED;
	int32_t *state2 = wl_array_add(&array, sizeof(int32_t));
	*state2 = XDG_TOPLEVEL_STATE_MAXIMIZED;
	struct box box = screen_get_dimensions(server_get_screen(server));
/*
 * XXX: mpv --gpu-context=wayland segfaults for this, probably their bug:
 *      they try to resize the window before initializing egl_window
 * The following `if` is just a temporary way to avoid this segfault
 */
	if (strcmp(toplevel_data->app_id, "mpv"))
	xdg_toplevel_send_configure(resource, box.width, box.height, &array);
//	xdg_toplevel_send_configure(resource, 500, 500, &array);
	server_set_focus(toplevel_data); // TODO: temp !!!!
}

char *xdg_toplevel_get_app_id(struct xdg_toplevel_data *data) {
	return data->app_id;
}
