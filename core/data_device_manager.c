#include <inttypes.h>
#include <wayland-server-protocol.h>

#include <core/data_device.h>
#include <core/data_source.h>

void data_device_manager_create_data_source(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {
	struct wl_resource *data_source_resource = wl_resource_create(client,
	&wl_data_source_interface, 1, id);
	data_source_new(data_source_resource);
}

void data_device_manager_get_data_device(struct wl_client *client, struct
wl_resource *resource, uint32_t id, struct wl_resource *seat) {
	struct wl_resource *data_device_resource = wl_resource_create(client,
	&wl_data_device_interface, 1, id);
	data_device_new(data_device_resource);
}

static const struct wl_data_device_manager_interface impl = {
	.create_data_source = data_device_manager_create_data_source,
	.get_data_device = data_device_manager_get_data_device
};

void data_device_manager_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
