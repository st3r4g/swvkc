#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-protocol.h>

#include <core/data_device.h>
#include <core/data_offer.h>

extern struct wl_client *focus;
extern struct wl_resource *selection;

static void start_drag(struct wl_client *client, struct wl_resource *resource,
struct wl_resource *source, struct wl_resource *origin, struct
wl_resource *icon, uint32_t serial) {

}

static enum wl_iterator_result set_selection_all(struct wl_resource *resource,
void *user_data) {
	if (!strcmp(wl_resource_get_class(resource), "wl_data_device")) {
		struct data_device *data = wl_resource_get_user_data(resource);
/*		struct data_offer *node, *tmp;
		wl_list_for_each_safe(node, tmp, &data->data_offer_list, link) {
			wl_resource_destroy(node->resource);
		}*/
		struct wl_client *client = wl_resource_get_client(resource);
		struct wl_resource *source = user_data;
		struct wl_resource *offer = wl_resource_create(client,
		 &wl_data_offer_interface, 3, 0);
		wl_data_device_send_data_offer(resource, offer);
		struct data_offer *new = data_offer_new(offer, source);
		wl_list_insert(&data->data_offer_list, &new->link);
		wl_data_device_send_selection(resource, offer);
	}
	return WL_ITERATOR_CONTINUE;
}

static enum wl_iterator_result cancel(struct wl_resource *resource,
void *user_data) {
	if (!strcmp(wl_resource_get_class(resource), "wl_data_source")) {
		wl_data_source_send_cancelled(resource);
	}
	return WL_ITERATOR_CONTINUE;
}

static void set_selection(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *source, uint32_t serial) {
	// TODO: serial verification
	selection = source;

/*	struct wl_display *display = wl_client_get_display(client);
	struct wl_list *list = wl_display_get_client_list(display);
	struct wl_client *client_;
	wl_client_for_each(client_, list) {
		wl_client_for_each_resource(client_, cancel, NULL);
	}*/
	// maybe avoid looping over all resources by a more fine-grained method
	wl_client_for_each_resource(focus, set_selection_all, source);

//	struct wl_display *display = wl_client_get_display(client);
//	struct wl_list *list = wl_display_get_client_list(display);
//	struct wl_client *client_;
//	wl_client_for_each(client_, list) {
	/*
	 * For some reason Firefox wants the selection to be set on a different
	 * `wl_data_device` resource than the one calling this
	 */
//	}
//	TODO: The mechanism for inter-client clipboard works but we must design
//	a better way to do it:
//	1) Advertise the selection on focus too
//	2) Watch out for `wl_data_source` destruction events
}

static void release(struct wl_client *client, struct wl_resource *resource) {

}

static const struct wl_data_device_interface impl = {
	.start_drag = start_drag,
	.set_selection = set_selection,
	.release = release
};

static void data_device_free(struct wl_resource *resource) {
	struct data_device *data = wl_resource_get_user_data(resource);
	wl_list_remove(&data->link);
	free(data);
}

struct data_device *data_device_new(struct wl_resource *resource) {
	struct data_device *data = calloc(1, sizeof(struct data_device));
	data->resource = resource;
	wl_list_init(&data->data_offer_list);

	wl_resource_set_implementation(resource, &impl, data, data_device_free);
	return data;
}
