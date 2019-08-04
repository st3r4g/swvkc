#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include <linux-dmabuf-unstable-v1-server-protocol.h>
#include <wayland-server-protocol.h>

#include <extensions/linux-dmabuf-unstable-v1/zwp_linux_buffer_params_v1.h>
#include <extensions/linux-dmabuf-unstable-v1/wl_buffer_dmabuf.h>

uint32_t format_get_plane_number(uint32_t format) {
	switch (format) {
		default:
		return 1;
	}
}

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void add(struct wl_client *client, struct wl_resource *resource, int32_t
fd, uint32_t plane_idx, uint32_t offset, uint32_t stride, uint32_t modifier_hi,
uint32_t modifier_lo) {
	struct zwp_linux_buffer_params_v1_data* data =
	wl_resource_get_user_data(resource);

	struct plane *plane = malloc(sizeof(struct plane));
	plane->fd = fd;
	plane->plane_idx = plane_idx;
	plane->offset = offset;
	plane->stride = stride;
	plane->modifier = ((uint64_t)modifier_hi << 32) | modifier_lo; //TODO: check this
	wl_list_insert(&data->plane_list, &plane->link);
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

void create_(struct wl_client *client, struct wl_resource *resource, uint32_t
buffer_id, int32_t width, int32_t height, uint32_t format, uint32_t flags) {
	struct zwp_linux_buffer_params_v1_data* params =
	wl_resource_get_user_data(resource);
	struct wl_resource *child = wl_resource_create(client,
	&wl_buffer_interface, 1, buffer_id);
	struct wl_buffer_dmabuf_data *dmabuf = wl_buffer_dmabuf_new(child);
	dmabuf->width = width;
	dmabuf->height = height;
	dmabuf->format = format;
	dmabuf->flags = flags;

	dmabuf->num_planes = format_get_plane_number(format);
	/* move allocations inside wl_buffer_dmabuf_new */
	dmabuf->fds = malloc(dmabuf->num_planes*sizeof(int32_t));
	dmabuf->offsets = malloc(dmabuf->num_planes*sizeof(uint32_t));
	dmabuf->strides = malloc(dmabuf->num_planes*sizeof(uint32_t));
	dmabuf->modifiers = malloc(dmabuf->num_planes*sizeof(uint64_t));

	struct plane *plane;
	wl_list_for_each(plane, &params->plane_list, link) {
		uint32_t i = plane->plane_idx;
		if (i >= dmabuf->num_planes) {
			// post error
			return;
		}
		dmabuf->fds[i] = plane->fd;
		dmabuf->offsets[i] = plane->offset;
		dmabuf->strides[i] = plane->stride;
		dmabuf->modifiers[i] = plane->modifier;
	}

	if (buffer_id == 0) // `create` request
		zwp_linux_buffer_params_v1_send_created(resource, child);
}

static void create(struct wl_client *client, struct wl_resource *resource,
int32_t width, int32_t height, uint32_t format, uint32_t flags) {
	create_(client, resource, 0, width, height, format, flags);
}

static void create_immed(struct wl_client *client, struct wl_resource *resource,
uint32_t buffer_id, int32_t width, int32_t height, uint32_t format, uint32_t
flags) {
	create_(client, resource, buffer_id, width, height, format, flags);
}

static const struct zwp_linux_buffer_params_v1_interface impl = {destroy,
add, create, create_immed};

static void free_data(struct wl_resource *resource) {
	struct zwp_linux_buffer_params_v1_data* data =
	wl_resource_get_user_data(resource);
	struct plane *plane, *tmp;
	wl_list_for_each_safe(plane, tmp, &data->plane_list, link) {
		wl_list_remove(&plane->link);
		free(plane);
	}
	free(data);
}

struct zwp_linux_buffer_params_v1_data *zwp_linux_buffer_params_v1_new(struct
wl_resource *resource) {
	struct zwp_linux_buffer_params_v1_data* data = malloc(sizeof(*data));
	wl_list_init(&data->plane_list);
	wl_resource_set_implementation(resource, &impl, data, free_data);
	return data;
}
