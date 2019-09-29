#include <inttypes.h>
#include <wayland-server-protocol.h>

#include <core/data_offer.h>
#include <core/data_source.h>

static void start_drag(struct wl_client *client, struct wl_resource *resource,
struct wl_resource *source, struct wl_resource *origin, struct
wl_resource *icon, uint32_t serial) {

}

static int i = 0;

static void set_selection(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *source, uint32_t serial) {
	if (source == NULL || i > 0)
		return;

	struct wl_resource *offer = wl_resource_create(client,
	 &wl_data_offer_interface, 3, 0); // TODO: version
	wl_data_device_send_data_offer(resource, offer);
	data_offer_new(offer, source);
	wl_data_device_send_selection(resource, offer);
	i++;
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
