#include <wayland-server-core.h>
#include <server.h>

struct xdg_toplevel_data {
	struct xdg_surface0 *xdg_surface_data;
	struct wl_listener commit;
	struct server *server;
	char *app_id;
/*
 * The following violates the concept that each resource user_data should only
 * contain information about that resource, but it's the most convenient way to
 * mantain a list of windows as long as the xdg_shell is the only shell
 */
	struct wl_list link;
};

void xdg_toplevel_new(struct wl_resource *, struct xdg_surface0 *, struct server
*);

char *xdg_toplevel_get_app_id(struct xdg_toplevel_data *);
