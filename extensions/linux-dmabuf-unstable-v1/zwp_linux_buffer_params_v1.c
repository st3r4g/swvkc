#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include <protocols/linux-dmabuf-unstable-v1-server-protocol.h>
#include <wayland-server-protocol.h>

#include <extensions/linux-dmabuf-unstable-v1/zwp_linux_buffer_params_v1.h>
#include <extensions/linux-dmabuf-unstable-v1/wl_buffer_dmabuf.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void add(struct wl_client *client, struct wl_resource *resource, int32_t
fd, uint32_t plane_idx, uint32_t offset, uint32_t stride, uint32_t modifier_hi,
uint32_t modifier_lo) {
	struct zwp_linux_buffer_params_v1_data* data =
	wl_resource_get_user_data(resource);
	data->fd = fd;
	data->plane_idx = plane_idx;
	data->offset = offset;
	data->stride = stride;
	data->modifier = ((uint64_t)modifier_hi << 32) | modifier_lo; //TODO: check this
/*	const int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	unsigned char *buffer = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (buffer == MAP_FAILED) {
		perror("ERROR");
		return;
	}
	for (int i=5504*768-60; i<5504*768-30; i++)
		errlog("[%d] byte %i: %x", size, i, buffer[i]);
//	errlog("size %d", );
//	FILE* file = fopen("image.data", "w");
//	fwrite(buffer, size, 1, file);
//	fclose(file);
	munmap(buffer, size);
//	free(buffer);*/
	/*const int size = 1366*768*4;
	char *buffer = malloc(size);
	read(fd, buffer, 256);
	for (int i=0; i<256; i++)
		errlog("%d", buffer[i]);
	free(buffer);*/
}

static void create(struct wl_client *client, struct wl_resource *resource,
int32_t width, int32_t height, uint32_t format, uint32_t flags) {

}

static void create_immed(struct wl_client *client, struct wl_resource *resource,
uint32_t buffer_id, int32_t width, int32_t height, uint32_t format, uint32_t
flags) {
	struct zwp_linux_buffer_params_v1_data* params =
	wl_resource_get_user_data(resource);
	struct wl_resource *child = wl_resource_create(client,
	&wl_buffer_interface, 1, buffer_id);
	struct wl_buffer_dmabuf_data *dmabuf = wl_buffer_dmabuf_new(child);
	dmabuf->width = width;
	dmabuf->height = height;
	dmabuf->format = format;
	dmabuf->flags = flags;

	dmabuf->fd = params->fd;
	dmabuf->plane_idx = params->plane_idx;
	dmabuf->offset = params->offset;
	dmabuf->stride = params->stride;
	dmabuf->modifier = params->modifier;
}

static const struct zwp_linux_buffer_params_v1_interface impl = {destroy,
add, create, create_immed};

static void free_data(struct wl_resource *resource) {
	struct zwp_linux_buffer_params_v1_data* data =
	wl_resource_get_user_data(resource);
	free(data);
}

struct zwp_linux_buffer_params_v1_data *zwp_linux_buffer_params_v1_new(struct
wl_resource *resource) {
	struct zwp_linux_buffer_params_v1_data* data = malloc(sizeof(*data));
	wl_resource_set_implementation(resource, &impl, data, free_data);
	return data;
}
