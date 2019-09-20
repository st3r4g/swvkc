#define _POSIX_C_SOURCE 200809L

#include <extensions/linux-dmabuf-unstable-v1/zwp_linux_dmabuf_v1.h>
#include <extensions/linux-dmabuf-unstable-v1/zwp_linux_buffer_params_v1.h>

#include <linux-dmabuf-unstable-v1-server-protocol.h>

#include <libdrm/drm_fourcc.h>

#include <stdlib.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void create_params(struct wl_client *client, struct wl_resource
*resource, uint32_t params_id) {
	struct linux_dmabuf *linux_dmabuf = wl_resource_get_user_data(resource);
	int version = wl_resource_get_version(resource);
	struct wl_resource *child = wl_resource_create(client,
	&zwp_linux_buffer_params_v1_interface, version, params_id);
	zwp_linux_buffer_params_v1_new(child, linux_dmabuf->buffer_dmabuf_events);
}

static const struct zwp_linux_dmabuf_v1_interface impl = {destroy,
create_params};

void send_format(struct wl_resource *resource, uint32_t format, uint64_t modifier) {
	uint32_t hi = modifier >> 32;
	uint32_t lo = modifier & 0xFFFFFFFF;
	zwp_linux_dmabuf_v1_send_modifier(resource, format, hi, lo);
}

void linux_dmabuf_free(struct wl_resource *resource) {
	struct linux_dmabuf *linux_dmabuf = wl_resource_get_user_data(resource);
	free(linux_dmabuf);
}

void zwp_linux_dmabuf_v1_new(struct wl_resource *resource, struct
buffer_dmabuf_events buffer_dmabuf_events) {
	struct linux_dmabuf *linux_dmabuf = malloc(sizeof(struct linux_dmabuf));
	linux_dmabuf->buffer_dmabuf_events = buffer_dmabuf_events;
	wl_resource_set_implementation(resource, &impl, linux_dmabuf,
	 linux_dmabuf_free);
/*
 * Some common formats (Intel GPU)
 */
	if (wl_resource_get_version(resource) >= 3) {
		send_format(resource, DRM_FORMAT_XRGB8888, DRM_FORMAT_MOD_LINEAR);
		send_format(resource, DRM_FORMAT_XRGB8888, I915_FORMAT_MOD_X_TILED);
		send_format(resource, DRM_FORMAT_ARGB8888, DRM_FORMAT_MOD_LINEAR);
		send_format(resource, DRM_FORMAT_ARGB8888, I915_FORMAT_MOD_X_TILED);
	} else {
		zwp_linux_dmabuf_v1_send_format(resource, DRM_FORMAT_XRGB8888);
		zwp_linux_dmabuf_v1_send_format(resource, DRM_FORMAT_ARGB8888);
		zwp_linux_dmabuf_v1_send_format(resource, DRM_FORMAT_YUYV);
	}
}
