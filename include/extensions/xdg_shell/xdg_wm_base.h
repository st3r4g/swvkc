#ifndef MYXDGWMBASE_H
#define MYXDGWMBASE_H

#include <extensions/xdg_shell/xdg_surface.h>
#include <extensions/xdg_shell/xdg_toplevel.h>

struct xdg_wm_base {
	int xdg_surface_count;
	struct xdg_toplevel_events xdg_toplevel_events;
	struct server *server;
};

struct xdg_wm_base *xdg_wm_base_new(struct wl_resource *resource, struct server
*server, struct xdg_toplevel_events xdg_toplevel_events);

#endif
