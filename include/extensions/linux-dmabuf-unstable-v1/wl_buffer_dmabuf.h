#include <stdbool.h>

struct wl_buffer_dmabuf_data {
	int32_t width;
	int32_t height;
	uint32_t format;
	uint32_t flags;

	uint32_t num_planes;
	int32_t *fds;
	uint32_t *offsets;
	uint32_t *strides;
	uint64_t *modifiers;
};

struct wl_buffer_dmabuf_data *wl_buffer_dmabuf_new(struct wl_resource *);

bool wl_buffer_is_dmabuf(struct wl_resource *);

int32_t wl_buffer_dmabuf_get_width(struct wl_resource *);
int32_t wl_buffer_dmabuf_get_height(struct wl_resource *);
uint32_t wl_buffer_dmabuf_get_format(struct wl_resource *);
uint32_t wl_buffer_dmabuf_get_num_planes(struct wl_resource *);
int32_t *wl_buffer_dmabuf_get_fds(struct wl_resource *);
uint32_t *wl_buffer_dmabuf_get_offsets(struct wl_resource *);
uint32_t *wl_buffer_dmabuf_get_strides(struct wl_resource *);
uint64_t *wl_buffer_dmabuf_get_mods(struct wl_resource *);
