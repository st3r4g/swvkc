#define _POSIX_C_SOURCE 200809L

#include <protocols/xdg-shell-server-protocol.h>

static void destroy(struct wl_client *client, struct wl_resource *resource) {

}

static void set_size(struct wl_client *client, struct wl_resource *resource,
int32_t width, int32_t height) {

}

static void set_anchor_rect(struct wl_client *client, struct wl_resource
*resource, int32_t x, int32_t y, int32_t width, int32_t height) {

}

static void set_anchor(struct wl_client *client, struct wl_resource *resource,
uint32_t anchor) {

}

static void set_gravity(struct wl_client *client, struct wl_resource *resource,
uint32_t gravity) {

}

static void set_constraint_adjustment(struct wl_client *client, struct
wl_resource *resource, uint32_t constraint_adjustment) {

}

static void set_offset(struct wl_client *client, struct wl_resource *resource,
int32_t x, int32_t y) {

}

static const struct xdg_positioner_interface impl = {
	.destroy = destroy,
	.set_size = set_size,
	.set_anchor_rect = set_anchor_rect,
	.set_anchor = set_anchor,
	.set_gravity = set_gravity,
	.set_constraint_adjustment = set_constraint_adjustment,
	.set_offset = set_offset,
};

// version 1

void xdg_positioner_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
