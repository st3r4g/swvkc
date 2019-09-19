#define _POSIX_C_SOURCE 200809L
#include <wayland-server-core.h>

#include <linux-explicit-synchronization-unstable-v1-server-protocol.h>

void linux_buffer_release_new(struct wl_resource *resource) {
	zwp_linux_buffer_release_v1_send_immediate_release(resource);
	wl_resource_destroy(resource);
}
