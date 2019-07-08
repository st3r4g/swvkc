#include <stdbool.h>

struct wl_buffer_dmabuf_data {
	int32_t width;
	int32_t height;
	uint32_t format;
	uint32_t flags;

	int32_t fd;
	uint32_t plane_idx;
	uint32_t offset;
	uint32_t stride;
	uint64_t modifier;
};

struct wl_buffer_dmabuf_data *wl_buffer_dmabuf_new(struct wl_resource *);

bool wl_buffer_is_dmabuf(struct wl_resource *);

int32_t wl_buffer_dmabuf_get_width(struct wl_resource *);
int32_t wl_buffer_dmabuf_get_height(struct wl_resource *);
uint32_t wl_buffer_dmabuf_get_format(struct wl_resource *);
int32_t wl_buffer_dmabuf_get_fd(struct wl_resource *);
uint32_t wl_buffer_dmabuf_get_offset(struct wl_resource *);
uint32_t wl_buffer_dmabuf_get_stride(struct wl_resource *);
uint64_t wl_buffer_dmabuf_get_mod(struct wl_resource *);
uint32_t wl_buffer_dmabuf_get_mod_lo(struct wl_resource *);
uint32_t wl_buffer_dmabuf_get_mod_hi(struct wl_resource *);
