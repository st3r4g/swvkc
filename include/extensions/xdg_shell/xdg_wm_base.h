#ifndef MYXDGWMBASE_H
#define MYXDGWMBASE_H

#include <extensions/xdg_shell/xdg_surface.h>

struct xdg_shell_events {
	xdg_surface_contents_update_t xdg_surface_contents_update;

	void *user_data;
};

struct xdg_wm_base {
	int xdg_surface_count;
	struct xdg_shell_events xdg_shell_events;
	struct server *server;
};

struct xdg_wm_base *xdg_wm_base_new(struct wl_resource *resource, struct server
*server, struct xdg_shell_events xdg_shell_events);

#endif
