#include <wayland-server-core.h>
#include <server.h>

struct xdg_toplevel_data {
	struct wl_listener commit;
	struct server *server;
};

void xdg_toplevel_new(struct wl_resource *, struct wl_resource *, struct server
*);
