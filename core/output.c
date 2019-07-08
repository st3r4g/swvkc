#define _POSIX_C_SOURCE 200809L
#include <core/output.h>
#include <wayland-server-protocol.h>

static void output_release(struct wl_client *client, struct wl_resource
*resource) {

}

static const struct wl_output_interface impl = {
	.release = output_release
};

void output_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
	wl_output_send_geometry(resource, 0, 0, 0, 0,
	WL_OUTPUT_SUBPIXEL_UNKNOWN, "aaa", "bbb",
	WL_OUTPUT_TRANSFORM_NORMAL);
	wl_output_send_mode(resource, WL_OUTPUT_MODE_CURRENT |
	WL_OUTPUT_MODE_PREFERRED, 1366, 768, 60000);
	wl_output_send_scale(resource, 1);
	wl_output_send_done(resource);
}
