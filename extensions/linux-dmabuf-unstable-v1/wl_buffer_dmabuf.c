#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <unistd.h>

#include <wayland-server-protocol.h>

#include <extensions/linux-dmabuf-unstable-v1/wl_buffer_dmabuf.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static const struct wl_buffer_interface impl = {destroy};

static void free_data(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	close(data->fd);
	free(data);
}

struct wl_buffer_dmabuf_data *wl_buffer_dmabuf_new(struct wl_resource *resource)
{
	struct wl_buffer_dmabuf_data *data = malloc(sizeof(*data));
	wl_resource_set_implementation(resource, &impl, data, free_data);
	return data;
}

bool wl_buffer_is_dmabuf(struct wl_resource *resource) {
	return wl_resource_instance_of(resource, &wl_buffer_interface, &impl);
}

int32_t wl_buffer_dmabuf_get_width(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->width;
}

int32_t wl_buffer_dmabuf_get_height(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->height;
}

uint32_t wl_buffer_dmabuf_get_format(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->format;
}

int32_t wl_buffer_dmabuf_get_fd(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->fd;
}

uint32_t wl_buffer_dmabuf_get_offset(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->offset;
}

uint32_t wl_buffer_dmabuf_get_stride(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->stride;
}

uint64_t wl_buffer_dmabuf_get_mod(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->modifier;
}

uint32_t wl_buffer_dmabuf_get_mod_lo(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->modifier & 0xFFFFFFFF;
}

uint32_t wl_buffer_dmabuf_get_mod_hi(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->modifier >> 32;
}
