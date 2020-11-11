#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <wayland-server-protocol.h>

#include <core/data_source.h>
#include <core/data_offer.h>
#include <util/log.h>

static void offer(struct wl_client *client, struct wl_resource *resource, const
char *mime_type) {
	struct data_source *data_source = wl_resource_get_user_data(resource);
	struct mime_node *node = malloc(sizeof(struct mime_node));
	node->mime_type = strdup(mime_type);
	wl_list_insert(&data_source->mime_list, &node->link);
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

// 29-10 TODO: remove this code duplicate from data_device.c
static enum wl_iterator_result set_selection_all(struct wl_resource *resource,
void *user_data) {
	if (wl_resource_get_id(resource) == 1) return WL_ITERATOR_CONTINUE;
	errlog("%d", wl_resource_get_id(resource));
	if (!strcmp(wl_resource_get_class(resource), "wl_data_device")) {
		struct wl_client *client = wl_resource_get_client(resource);
		struct wl_resource *source = user_data;
		// 29-10 advertise no selection
		if (source == NULL) {
			wl_data_device_send_selection(resource, NULL);
		} else {
		struct wl_resource *offer = wl_resource_create(client,
		 &wl_data_offer_interface, 3, 0);
		wl_data_device_send_data_offer(resource, offer);
		data_offer_new(offer, source);
		wl_data_device_send_selection(resource, offer);
		data_source_bind_offer(source, offer);
		}
	}
	return WL_ITERATOR_CONTINUE;
}

void data_source_free(struct wl_resource *resource) {
	struct data_source *data_source = wl_resource_get_user_data(resource);
	struct mime_node *node, *tmp;
	wl_list_for_each_safe(node, tmp, &data_source->mime_list, link) {
		wl_list_remove(&node->link);
		free(node->mime_type);
		free(node);
	}
	struct offer_node *node_, *tmp_;
	wl_list_for_each_safe(node_, tmp_, &data_source->offer_list, link) {
		data_offer_invalidate(node_->offer);
		wl_list_remove(&node_->link);
		free(node_);
	}
	// 29-10 advertise no selection
	struct wl_client *client = wl_resource_get_client(resource);
	struct wl_display *display = wl_client_get_display(client);
	struct wl_list *list = wl_display_get_client_list(display);
	struct wl_client *client_;
	wl_client_for_each(client_, list) {
		wl_client_for_each_resource(client_, set_selection_all, NULL);
	}
	free(data_source);
}

void data_source_new(struct wl_resource *resource) {
	struct data_source *data_source = malloc(sizeof(struct data_source));
	wl_list_init(&data_source->mime_list);
	wl_list_init(&data_source->offer_list);
	wl_resource_set_implementation(resource, &impl, data_source, data_source_free);
}

void data_source_bind_offer(struct wl_resource *resource, struct wl_resource
*offer) {
	struct data_source *data_source = wl_resource_get_user_data(resource);
	struct offer_node *node = malloc(sizeof(struct offer_node));
	node->offer = offer;
	wl_list_insert(&data_source->offer_list, &node->link);
}
