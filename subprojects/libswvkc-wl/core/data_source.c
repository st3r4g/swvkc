#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <wayland-server-protocol.h>

struct mime_node {
	const char *mime_type;
	struct wl_list link;
};

static void offer(struct wl_client *client, struct wl_resource *resource, const
char *mime_type) {
	struct wl_list *list = wl_resource_get_user_data(resource);
	struct mime_node *node = malloc(sizeof(struct mime_node));
	node->mime_type = strdup(mime_type);
	wl_list_insert(list, &node->link);
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
	struct wl_list *mime = malloc(sizeof(struct wl_list));
	wl_list_init(mime);
	wl_resource_set_implementation(resource, &impl, mime, 0);
	// TODO: set mime list destructor
}
