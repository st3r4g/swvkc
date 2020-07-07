#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <wayland-server-protocol.h>

#include <core/data_source.h>

extern struct wl_resource *selection;

static void offer(struct wl_client *client, struct wl_resource *resource, const
char *mime_type) {
	struct data_source *me = wl_resource_get_user_data(resource);
	struct mime_node *node = malloc(sizeof(struct mime_node));
	node->mime_type = strdup(mime_type);
	wl_list_insert(&me->list, &node->link);
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

#include <stdio.h>

static void data_source_free(struct wl_resource *resource) {
	struct data_source *me = wl_resource_get_user_data(resource);
	// If selection is me, invalidate (is it right?)
	if (selection == resource) {
		printf("selection is me\n");
		selection = NULL;
	} else
		printf("selection is NOT me\n");
	struct mime_node *node, *tmp;
	wl_list_for_each_safe(node, tmp, &me->list, link) {
		wl_list_remove(&node->link);
		free(node->mime_type);
		free(node);
	}
	wl_list_remove(&me->link);
	free(me);
	// TODO: send info to data_device to destroy all offers created by me?
}

struct data_source *data_source_new(struct wl_resource *resource) {
	struct data_source *me = calloc(1, sizeof(struct data_source));
	me->resource = resource;
	wl_list_init(&me->list);

	wl_resource_set_implementation(resource, &impl, me, data_source_free);
	return me;
}
