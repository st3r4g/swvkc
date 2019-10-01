#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-server-protocol.h>

#include <core/data_source.h>

static void accept(struct wl_client *client, struct wl_resource *resource,
uint32_t serial, const char *mime_type) {

}

static void receive(struct wl_client *client, struct wl_resource *resource,
const char *mime_type, int32_t fd) {
	struct wl_resource *source = wl_resource_get_user_data(resource);
	wl_data_source_send_send(source, mime_type, fd);
	close(fd);
}

static void destroy(struct wl_client *client, struct wl_resource *resource) {

}

static void finish(struct wl_client *client, struct wl_resource *resource) {

}

static void set_actions(struct wl_client *client, struct wl_resource *resource,
uint32_t dnd_actions, uint32_t preferred_action) {

}

static const struct wl_data_offer_interface impl = {
	.accept = accept,
	.receive = receive,
	.destroy = destroy,
	.finish = finish,
	.set_actions = set_actions
};

void data_offer_new(struct wl_resource *resource, struct wl_resource *source) {
	wl_resource_set_implementation(resource, &impl, source, 0);
	struct wl_list *list = wl_resource_get_user_data(source);
	struct mime_node *node;
	wl_list_for_each_reverse(node, list, link)
		wl_data_offer_send_offer(resource, node->mime_type);
}
