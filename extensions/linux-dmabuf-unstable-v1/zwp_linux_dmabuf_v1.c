#define _POSIX_C_SOURCE 200809L

#include <libdrm/drm_fourcc.h>

#include <linux-dmabuf-unstable-v1-server-protocol.h>
#include <extensions/linux-dmabuf-unstable-v1/zwp_linux_buffer_params_v1.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void create_params(struct wl_client *client, struct wl_resource
*resource, uint32_t params_id) {
	int version = wl_resource_get_version(resource);
	struct wl_resource *child = wl_resource_create(client,
	&zwp_linux_buffer_params_v1_interface, version, params_id);
	zwp_linux_buffer_params_v1_new(child);
}

static const struct zwp_linux_dmabuf_v1_interface impl = {destroy,
create_params};

void zwp_linux_dmabuf_v1_new(struct wl_resource *resource, bool dmabuf_mod) {
	wl_resource_set_implementation(resource, &impl, NULL, NULL);

	zwp_linux_dmabuf_v1_send_modifier(resource, DRM_FORMAT_XRGB8888, 0, 0);
	if (dmabuf_mod) {
		zwp_linux_dmabuf_v1_send_modifier(resource, DRM_FORMAT_XRGB8888, 16777216, 1);
	}
//	zwp_linux_dmabuf_v1_send_modifier(resource, 875713089, 0, 0);//2?
//	zwp_linux_dmabuf_v1_send_modifier(resource, DRM_FORMAT_XBGR8888, 0, 0);//2?
//	zwp_linux_dmabuf_v1_send_modifier(resource, DRM_FORMAT_XBGR8888, 16777216, 1);//2?
//	zwp_linux_dmabuf_v1_send_modifier(resource, DRM_FORMAT_XRGB8888, 16777216, 1);//2?
//	zwp_linux_dmabuf_v1_send_modifier(resource, 875713089, 16777216, 2);//2?
//	zwp_linux_dmabuf_v1_send_modifier(resource, DRM_FORMAT_ARGB8888, 16777216, 1);//2?
}
