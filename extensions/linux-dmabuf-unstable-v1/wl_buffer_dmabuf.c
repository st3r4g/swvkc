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
	for (size_t i=0; i<data->num_planes; i++)
		close(data->fds[i]);
	free(data->fds);
	free(data->offsets);
	free(data->strides);
	free(data->modifiers);
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

uint32_t wl_buffer_dmabuf_get_num_planes(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->num_planes;
}

int32_t *wl_buffer_dmabuf_get_fds(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->fds;
}

uint32_t *wl_buffer_dmabuf_get_offsets(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->offsets;
}

uint32_t *wl_buffer_dmabuf_get_strides(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->strides;
}

uint64_t *wl_buffer_dmabuf_get_mods(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->modifiers;
}

/*uint32_t wl_buffer_dmabuf_get_mod_lo(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->modifier & 0xFFFFFFFF;
}

uint32_t wl_buffer_dmabuf_get_mod_hi(struct wl_resource *resource) {
	struct wl_buffer_dmabuf_data *data = wl_resource_get_user_data(resource);
	return data->modifier >> 32;
}*/
