#define _POSIX_C_SOURCE 200809L
#include <swvkc.h>

#include <backend/screen.h>

#include <core/compositor.h>
#include <core/data_device_manager.h>
#include <core/output.h>
#include <core/seat.h>
#include <core/wl_subcompositor.h>
#include <extensions/xdg_shell/xdg_wm_base.h>
#include <extensions/linux-dmabuf-unstable-v1/zwp_linux_dmabuf_v1.h>
#include <extensions/linux-explicit-synchronization-v1/zwp_linux_explicit_synchronization_v1.h>
#include <extensions/fullscreen-shell-unstable-v1/zwp_fullscreen_shell_v1.h>
#include <extensions/server-decoration/org_kde_kwin_server_decoration_manager.h>

#include <util/util.h>

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <xdg-shell-server-protocol.h>
#include <linux-dmabuf-unstable-v1-server-protocol.h>
#include <linux-explicit-synchronization-unstable-v1-server-protocol.h>
#include <fullscreen-shell-unstable-v1-server-protocol.h>
#include <server-decoration-server-protocol.h>

#include <stdlib.h>
#include <string.h>

/*
 * Defined in main.c
 */
extern void surface_map_notify(struct surface *surface, void *user_data);
extern void surface_unmap_notify(struct surface *surface, void *user_data);
extern void surface_contents_update_notify(struct surface *surface, void *user_data);
extern void xdg_toplevel_init_notify(struct xdg_toplevel_data *xdg_toplevel, void
*user_data);
extern void buffer_dmabuf_create_notify(struct wl_buffer_dmabuf_data *dmabuf, void
*user_data);
extern void buffer_dmabuf_destroy_notify(struct wl_buffer_dmabuf_data *dmabuf, void
*user_data);
extern void keyboard_init_notify(struct keyboard *keyboard, void *user_data);

static void compositor_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&wl_compositor_interface, version, id);
	struct surface_events surface_events = {
		.map = surface_map_notify,
		.unmap = surface_unmap_notify,
		.contents_update = surface_contents_update_notify,
		.user_data = server
	};
	compositor_new(resource, surface_events);
}

static void data_device_manager_bind(struct wl_client *client, void *data,
uint32_t version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_compositor_interface, version, id);
	data_device_manager_new(resource);
}


static void seat_bind(struct wl_client *client, void *data, uint32_t version,
uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&wl_seat_interface, version, id);
	struct keyboard_events keyboard_events = {
		.init = keyboard_init_notify,
		.user_data = server
	};
	seat_new(resource, keyboard_events);
}

static void subcompositor_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_subcompositor_interface, version, id);
	wl_subcompositor_new(resource);
}

static void output_bind(struct wl_client *client, void *data, uint32_t version,
uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_output_interface, version, id);
	output_new(resource);
}

static void xdg_wm_base_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&xdg_wm_base_interface, version, id);
	struct xdg_toplevel_events xdg_toplevel_events = {
		.init = xdg_toplevel_init_notify,
		.user_data = server
	};
	xdg_wm_base_new(resource, server, xdg_toplevel_events);
}

static void zwp_linux_dmabuf_v1_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&zwp_linux_dmabuf_v1_interface, version, id);
	struct buffer_dmabuf_events buffer_dmabuf_events = {
		.create = buffer_dmabuf_create_notify,
		.destroy = buffer_dmabuf_destroy_notify,
		.user_data = server
	};
	zwp_linux_dmabuf_v1_new(resource, buffer_dmabuf_events);
}

static void zwp_linux_explicit_synchronization_v1_bind(struct wl_client *client,
void *data, uint32_t version, uint32_t id) {
//	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&zwp_linux_explicit_synchronization_v1_interface, version, id);
	linux_explicit_synchronization_new(resource);
}

static void zwp_fullscreen_shell_v1_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&zwp_fullscreen_shell_v1_interface, version, id);
	zwp_fullscreen_shell_v1_new(resource);
}

static void org_kde_kwin_server_decoration_manager_bind(struct wl_client
*client, void *data, uint32_t version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&org_kde_kwin_server_decoration_manager_interface, version, id);
	org_kde_kwin_server_decoration_manager_new(resource);
}

// For debugging
static bool global_filter(const struct wl_client *client, const struct wl_global
*global, void *data) {
	char *client_name = get_a_name((struct wl_client*)client);
	bool condition = wl_global_get_interface(global) == &zwp_fullscreen_shell_v1_interface;
	if (!strcmp(client_name, "weston-simple-d") && condition) {
		free(client_name);
		return false;
	}
	free(client_name);
	return true;
}

void create_globals(struct server *server, struct wl_display *D, bool dmabuf) {
	wl_global_create(D, &wl_compositor_interface, 4, server,
	compositor_bind);
	wl_global_create(D, &wl_subcompositor_interface, 1, 0,
	subcompositor_bind);
	wl_global_create(D, &wl_data_device_manager_interface, 1, 0,
	data_device_manager_bind);
	wl_global_create(D, &wl_seat_interface, 5, server, seat_bind);
	wl_global_create(D, &wl_output_interface, 3, 0, output_bind);
	wl_display_init_shm(D);
	wl_global_create(D, &xdg_wm_base_interface, 1, server,
	xdg_wm_base_bind);
	if (dmabuf)
		wl_global_create(D, &zwp_linux_dmabuf_v1_interface, 3, server,
		 zwp_linux_dmabuf_v1_bind);
	wl_global_create(D, &zwp_linux_explicit_synchronization_v1_interface, 1,
	server, zwp_linux_explicit_synchronization_v1_bind);
	wl_global_create(D, &zwp_fullscreen_shell_v1_interface, 1, NULL,
	zwp_fullscreen_shell_v1_bind);
	wl_global_create(D, &org_kde_kwin_server_decoration_manager_interface,
	1, NULL, org_kde_kwin_server_decoration_manager_bind);

//	wl_display_set_global_filter(D, global_filter, 0);
}
