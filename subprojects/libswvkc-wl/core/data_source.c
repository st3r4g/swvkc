#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <wayland-server-protocol.h>

#include <core/data_source.h>

static void offer(struct wl_client *client, struct wl_resource *resource, const
char *mime_type) {
	struct wl_list *list = wl_resource_get_user_data(resource);
	struct mime_node *node = malloc(sizeof(struct mime_node));
	node->mime_type = strdup(mime_type);
	wl_list_insert(list, &node->link);
}

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void set_actions(struct wl_client *client, struct wl_resource *resource,
uint32_t dnd_actions) {

}

static const struct wl_data_source_interface impl = {
	.offer = offer,
	.destroy = destroy,
	.set_actions = set_actions
};

void data_source_free(struct wl_resource *resource) {
	struct wl_list *list = wl_resource_get_user_data(resource);
	struct mime_node *node, *tmp;
	wl_list_for_each_safe(node, tmp, list, link) {
		wl_list_remove(&node->link);
		free(node->mime_type);
		free(node);
	}
	free(list);
}

void data_source_new(struct wl_resource *resource) {
	struct wl_list *list = malloc(sizeof(struct wl_list));
	wl_list_init(list);
	wl_resource_set_implementation(resource, &impl, list, data_source_free);
}
