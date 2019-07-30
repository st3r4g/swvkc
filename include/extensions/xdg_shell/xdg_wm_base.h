#ifndef MYXDGWMBASE_H
#define MYXDGWMBASE_H

#include <wayland-server-core.h>

struct xdg_wm_base {
//	struct wl_list xdg_surface_list; //remove
	struct wl_list xdg_toplevel_list;
	struct wl_list xdg_popup_list;
	struct wl_list link;

	struct server *server;

	struct wl_listener *xdg_surface_contents_update_listener;
};

struct xdg_wm_base *xdg_wm_base_new(struct wl_resource *resource, struct server
*server, struct wl_listener *xdg_surface_contents_update_listener);

#endif
