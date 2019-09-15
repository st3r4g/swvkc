#include <wayland-server-core.h>

#include <server-decoration-server-protocol.h>

#include <extensions/server-decoration/org_kde_kwin_server_decoration.h>

static void create(struct wl_client *client, struct wl_resource *resource,
uint32_t id, struct wl_resource *surface) {
	struct wl_resource *child = wl_resource_create(client,
	&org_kde_kwin_server_decoration_interface, 1, id);
	org_kde_kwin_server_decoration_new(child);
}

static const struct org_kde_kwin_server_decoration_manager_interface impl = {
	.create = create
};

void org_kde_kwin_server_decoration_manager_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
	org_kde_kwin_server_decoration_manager_send_default_mode(resource,
	 ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_SERVER);
}
