#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-server-protocol.h>
#include <stdio.h>

#include <core/data_offer.h>
#include <core/data_source.h>

static void accept(struct wl_client *client, struct wl_resource *resource,
uint32_t serial, const char *mime_type) {

}

static void receive(struct wl_client *client, struct wl_resource *resource,
const char *mime_type, int32_t fd) {
	struct data_offer *me = wl_resource_get_user_data(resource);
	wl_data_source_send_send(me->source, mime_type, fd);
	close(fd);
}

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
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

static void data_offer_free(struct wl_resource *resource) {
	printf("data_offer_free\n");
	struct data_offer *data = wl_resource_get_user_data(resource);
	wl_list_remove(&data->link);
	free(data);
}

struct data_offer *data_offer_new(struct wl_resource *resource, struct wl_resource *source) {
	struct data_offer *data = calloc(1, sizeof(struct data_offer));
	data->resource = resource;
	data->source = source;

	wl_resource_set_implementation(resource, &impl, data, data_offer_free); // TODO
	struct data_source *aaa = wl_resource_get_user_data(source);
	struct mime_node *node;
	wl_list_for_each_reverse(node, &aaa->list, link)
		wl_data_offer_send_offer(resource, node->mime_type);
	return data;
}
