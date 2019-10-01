#include <inttypes.h>
#include <string.h>
#include <wayland-server-protocol.h>

#include <core/data_offer.h>

static void start_drag(struct wl_client *client, struct wl_resource *resource,
struct wl_resource *source, struct wl_resource *origin, struct
wl_resource *icon, uint32_t serial) {

}

static enum wl_iterator_result set_selection_all(struct wl_resource *resource,
void *user_data) {
	if (!strcmp(wl_resource_get_class(resource), "wl_data_device")) {
		struct wl_client *client = wl_resource_get_client(resource);
		struct wl_resource *source = user_data;
		struct wl_resource *offer = wl_resource_create(client,
		 &wl_data_offer_interface, 3, 0);
		wl_data_device_send_data_offer(resource, offer);
		data_offer_new(offer, source);
		wl_data_device_send_selection(resource, offer);
	}
	return WL_ITERATOR_CONTINUE;
}

static void set_selection(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *source, uint32_t serial) {
	if (source == NULL)
		return;
	/*
	 * For some reason Firefox wants the selection to be set on a different
	 * `wl_data_device` resource than the one calling this
	 */
	wl_client_for_each_resource(client, set_selection_all, source);
}

static void release(struct wl_client *client, struct wl_resource *resource) {

}

static const struct wl_data_device_interface impl = {
	.start_drag = start_drag,
	.set_selection = set_selection,
	.release = release
};

void data_device_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
