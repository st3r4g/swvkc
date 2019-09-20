#include <inttypes.h>
#include <wayland-server-protocol.h>

static void start_drag(struct wl_client *client, struct wl_resource *resource,
struct wl_resource *source, struct wl_resource *origin, struct
wl_resource *icon, uint32_t serial) {

}


static void set_selection(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *source, uint32_t serial) {

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
