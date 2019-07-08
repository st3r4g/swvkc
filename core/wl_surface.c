#define _POSIX_C_SOURCE 200809L

#include <wayland-server-protocol.h>

#include <util/log.h>
#include <core/wl_surface.h>
#include <extensions/linux-dmabuf-unstable-v1/wl_buffer_dmabuf.h>
#include <backend/screen.h>
#include <server.h>

#include <stdlib.h>

enum staged_field {
	BUFFER = 1 << 0,
	DAMAGE = 1 << 1,
	OPAQUE_REGION = 1 << 2,
	INPUT_REGION = 1 << 3,
	BUFFER_TRANSFORM = 1 << 4,
	BUFFER_SCALE = 1 << 5,
	BUFFER_DAMAGE = 1 << 6
};

static void destroy(struct wl_client *client, struct wl_resource
*resource) {
	wl_resource_destroy(resource);
}

static void attach(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *buffer, int32_t x, int32_t y) {
// I didn't understand how to treat nonzero x and y
	if (x || y)
		errlog("surface.attach called with nonzero x=%d y=%d", x, y);
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->buffer = buffer;
	surface->staged |= BUFFER;
}

static void damage(struct wl_client *client, struct wl_resource
*resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->damage.x = x;
	surface->pending->damage.y = y;
	surface->pending->damage.width = width;
	surface->pending->damage.height = height;
	surface->staged |= DAMAGE;
}

static void frame(struct wl_client *client, struct wl_resource
*resource, uint32_t callback) {
	struct surface *surface = wl_resource_get_user_data(resource);
	if (!surface->frame) {
		surface->frame = wl_resource_create(client, &wl_callback_interface, 1, callback);
//		screen_vblank(server_get_screen(surface->server));
	}
}

static void set_opaque_region(struct wl_client *client, struct
wl_resource *resource, struct wl_resource *region) {
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->opaque_region = region;
	surface->staged |= OPAQUE_REGION;
}

static void set_input_region(struct wl_client *client, struct
wl_resource *resource, struct wl_resource *region) {
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->input_region = region;
	surface->staged |= INPUT_REGION;
}

static void commit(struct wl_client *client, struct wl_resource
*resource) {
	struct surface *surface = wl_resource_get_user_data(resource);
	struct dbuf_state *current = surface->current;
	struct dbuf_state *pending = surface->pending;

// "On commit, a pending wl_buffer is applied first, and all other state second"
	if (surface->staged & BUFFER) {
		current->buffer = pending->buffer;
//		renderer_delete_tex(surface->texture);
//		uint32_t buf_w, buf_h;
// "If wl_surface.attach is sent with a NULL wl_buffer, the following
//  wl_surface.commit will remove the surface content."
/*		if (current->buffer) {
//			surface->texture = tex_from_buffer(surface->egl,
//			current->buffer, &buf_w, &buf_h);
			struct wl_resource *buffer = current->buffer;
			uint32_t width = wl_buffer_dmabuf_get_width(buffer);
			uint32_t height = wl_buffer_dmabuf_get_height(buffer);
			uint32_t format = wl_buffer_dmabuf_get_format(buffer);
			int fd = wl_buffer_dmabuf_get_fd(buffer);
			int stride = wl_buffer_dmabuf_get_stride(buffer);
			int offset = wl_buffer_dmabuf_get_offset(buffer);
			uint64_t mod = wl_buffer_dmabuf_get_mod(buffer);
			screen_post_direct(server_get_screen(surface->server),
			width, height, format, fd, stride, offset, mod);
			wl_buffer_send_release(buffer);
		} else {
//			surface->texture = 0, buf_w = 0, buf_h = 0;
		}*/
	}

	if (surface->staged & DAMAGE)
		current->damage = pending->damage;

	if (surface->staged & OPAQUE_REGION)
		current->opaque_region = pending->opaque_region;

	if (surface->staged & INPUT_REGION)
		current->input_region = pending->input_region;

	if (surface->staged & BUFFER_TRANSFORM)
		current->buffer_transform = pending->buffer_transform;

	if (surface->staged & BUFFER_SCALE)
		current->buffer_scale = pending->buffer_scale;

	if (surface->staged & BUFFER_DAMAGE)
		current->buffer_damage = pending->buffer_damage;

	wl_signal_emit(&surface->commit, surface);
	surface->staged = 0;
}

static void set_buffer_transform(struct wl_client *client, struct
wl_resource *resource, int32_t transform) {
	if (transform < WL_OUTPUT_TRANSFORM_NORMAL ||
	    transform > WL_OUTPUT_TRANSFORM_FLIPPED_270) {
		wl_resource_post_error(resource,
		WL_SURFACE_ERROR_INVALID_TRANSFORM,
		"Buffer transform value %d is invalid", transform);
		return;
	}
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->buffer_transform = transform;
	surface->staged |= BUFFER_TRANSFORM;
}

static void set_buffer_scale(struct wl_client *client, struct
wl_resource *resource, int32_t scale) {
	if (scale < 1) {
		wl_resource_post_error(resource, WL_SURFACE_ERROR_INVALID_SCALE,
		"Buffer scale value %d is not positive", scale);
		return;
	}
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->buffer_scale = scale;
	surface->staged |= BUFFER_SCALE;
}

static void damage_buffer(struct wl_client *client, struct wl_resource
*resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->buffer_damage.x = x;
	surface->pending->buffer_damage.y = y;
	surface->pending->buffer_damage.width = width;
	surface->pending->buffer_damage.height = height;
	surface->staged |= BUFFER_DAMAGE;
}

static const struct wl_surface_interface impl = {
	destroy, attach, damage, frame, set_opaque_region, set_input_region,
	commit, set_buffer_transform, set_buffer_scale, damage_buffer
};

void surface_free(struct wl_resource *resource) {
	struct surface *surface = wl_resource_get_user_data(resource);
	free(surface->pending);
	free(surface->current);
	free(surface);
}

struct surface *surface_new(struct wl_resource *resource, struct server *server) {
	struct surface *surface = calloc(1, sizeof(struct surface));
	surface->pending = calloc(1, sizeof(struct dbuf_state));
	surface->current = calloc(1, sizeof(struct dbuf_state));

	// Buffered state initialization
	surface->current->buffer_transform = WL_OUTPUT_TRANSFORM_NORMAL;
	surface->current->buffer_scale = 1;

	surface->server = server;
	wl_signal_init(&surface->commit);
	wl_resource_set_implementation(resource, &impl, surface, surface_free);
	return surface;
}
