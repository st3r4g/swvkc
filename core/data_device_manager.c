#include <inttypes.h>
#include <wayland-server-protocol.h>

void data_device_manager_create_data_source(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {

}

void data_device_manager_get_data_device(struct wl_client *client, struct
wl_resource *resource, uint32_t id, struct wl_resource *seat) {

}

static const struct wl_data_device_manager_interface impl = {
	.create_data_source = data_device_manager_create_data_source,
	.get_data_device = data_device_manager_get_data_device
};

void data_device_manager_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
