#ifndef MYXDGWMBASE_H
#define MYXDGWMBASE_H

#include <wayland-server-core.h>
#include <extensions/xdg_shell/xdg_surface.h>

struct xdg_shell_events {
	xdg_surface_contents_update_t xdg_surface_contents_update;

	void *user_data;
};

struct xdg_wm_base {
//	struct wl_list xdg_surface_list; //remove
	struct wl_list xdg_toplevel_list;
	struct wl_list xdg_popup_list;
	struct wl_list link;

	struct server *server;

	struct xdg_shell_events xdg_shell_events;
};

struct xdg_wm_base *xdg_wm_base_new(struct wl_resource *resource, struct server
*server, struct xdg_shell_events xdg_shell_events);

#endif
