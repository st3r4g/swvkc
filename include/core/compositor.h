#include <wayland-server-core.h>

struct compositor {
	struct server *server;
};

struct compositor *compositor_new(struct wl_resource *resource, struct server *server);
