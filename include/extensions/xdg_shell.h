#ifndef MYXDGSHELL_H
#define MYXDGSHELL_H

#include <wayland-server-core.h>
#include <util/box.h>

struct xdg_surface_state {
	struct box window_geometry;
};

struct xdg_surface {
	struct wl_resource *surface;
	struct wl_resource *keyboard;
	struct xdg_surface_state *pending, *current;
	struct wl_listener commit;
	struct wl_list link;
	struct server *server;
};

struct xdg_shell {
	struct wl_list xdg_surface_list;
	struct wl_list link;
	struct server *server;
};

struct xdg_shell *xdg_shell_new(struct wl_resource *resource, struct server
*server);

#endif
