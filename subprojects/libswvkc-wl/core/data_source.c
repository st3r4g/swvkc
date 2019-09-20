#include <inttypes.h>
#include <wayland-server-protocol.h>

static void offer(struct wl_client *client, struct wl_resource *resource, const
char *mime_type) {

}

static void destroy(struct wl_client *client, struct wl_resource *resource) {

}

static void set_actions(struct wl_client *client, struct wl_resource *resource,
uint32_t dnd_actions) {

}

static const struct wl_data_source_interface impl = {
	.offer = offer,
	.destroy = destroy,
	.set_actions = set_actions
};

void data_source_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
