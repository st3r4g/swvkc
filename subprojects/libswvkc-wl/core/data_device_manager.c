#include <inttypes.h>
#include <stdlib.h>

#include <wayland-server-protocol.h>

#include <core/data_device_manager.h>
#include <core/data_device.h>
#include <core/data_source.h>

void data_device_manager_create_data_source(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {
	struct data_device_manager *data = wl_resource_get_user_data(resource);
	struct wl_resource *data_source_resource = wl_resource_create(client,
	&wl_data_source_interface, 3, id);
	struct data_source *new = data_source_new(data_source_resource);
	wl_list_insert(&data->data_source_list, &new->link);
}

void data_device_manager_get_data_device(struct wl_client *client, struct
wl_resource *resource, uint32_t id, struct wl_resource *seat) {
	struct data_device_manager *data = wl_resource_get_user_data(resource);
	struct wl_resource *data_device_resource = wl_resource_create(client,
	&wl_data_device_interface, 1, id);
	struct data_device *new = data_device_new(data_device_resource);
	wl_list_insert(&data->data_device_list, &new->link);
}

static const struct wl_data_device_manager_interface impl = {
	.create_data_source = data_device_manager_create_data_source,
	.get_data_device = data_device_manager_get_data_device
};

static void data_device_manager_free(struct wl_resource *resource) {
	struct data_device_manager *data = wl_resource_get_user_data(resource);
	wl_list_remove(&data->link);
	free(data);
}

struct data_device_manager *data_device_manager_new(struct wl_resource *resource) {
	struct data_device_manager *data = calloc(1, sizeof(struct data_device_manager));
	data->resource = resource;
	wl_list_init(&data->data_device_list);
	wl_list_init(&data->data_source_list);

	wl_resource_set_implementation(resource, &impl, data, data_device_manager_free);
	return data;
}
