#include <extensions/linux-dmabuf-unstable-v1/wl_buffer_dmabuf.h>

#include <stdbool.h>

struct linux_dmabuf {
	struct buffer_dmabuf_events buffer_dmabuf_events;
};

void zwp_linux_dmabuf_v1_new(struct wl_resource *resource, struct
buffer_dmabuf_events buffer_dmabuf_events);
