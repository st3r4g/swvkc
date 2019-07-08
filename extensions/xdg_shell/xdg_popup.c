#define _POSIX_C_SOURCE 200809L

#include <protocols/xdg-shell-server-protocol.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {

}

static void grab(struct wl_client *client, struct wl_resource *resource, struct
wl_resource *seat, uint32_t serial) {

}

static const struct xdg_popup_interface impl = {
	.destroy = destroy,
	.grab = grab
};

// version 1

void xdg_popup_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
