#include <wayland-server-core.h>

#include <server-decoration-server-protocol.h>

static void release(struct wl_client *client, struct wl_resource *resource) {

}

static void request_mode(struct wl_client *client, struct wl_resource *resource, uint32_t mode) {
	org_kde_kwin_server_decoration_send_mode(resource,
	 ORG_KDE_KWIN_SERVER_DECORATION_MODE_SERVER);
}

static const struct org_kde_kwin_server_decoration_interface impl = {
	.release = release,
	.request_mode = request_mode
};

void org_kde_kwin_server_decoration_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
	org_kde_kwin_server_decoration_send_mode(resource,
	 ORG_KDE_KWIN_SERVER_DECORATION_MODE_SERVER);
}
