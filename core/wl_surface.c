#define _POSIX_C_SOURCE 200809L

#include <core/wl_surface.h>
#include <core/wl_surface_staged_field.h>
#include <util/log.h>

#include <wayland-server-protocol.h>

#include <stdlib.h>

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
	if (surface->frame) {
		wl_callback_send_done(surface->frame, 0);
		wl_resource_destroy(surface->frame);
	}

	surface->frame = wl_resource_create(client, &wl_callback_interface, 1, callback);
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
	if (surface->staged & BUFFER)
		current->buffer = pending->buffer;

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
/*
 * Commit every other possible role-specific state
 */
	wl_signal_emit(&surface->commit, surface);

	if (surface->staged & BUFFER && surface->is_mapped)
		surface->surface_events.contents_update(surface,
		 surface->surface_events.user_data);

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
	errlog("surface destroyed");
	struct surface *surface = wl_resource_get_user_data(resource);
	if (surface->is_mapped) {
		surface->surface_events.unmap(surface,
		 surface->surface_events.user_data);
		surface->is_mapped = false;
	}
	free(surface->pending);
	free(surface->current);
	free(surface);
}

struct surface *surface_new(struct wl_resource *resource, struct surface_events
surface_events) {
	struct surface *surface = calloc(1, sizeof(struct surface));
	surface->pending = calloc(1, sizeof(struct dbuf_state));
	surface->current = calloc(1, sizeof(struct dbuf_state));

	surface->resource = resource;

	// Buffered state initialization
	surface->current->buffer_transform = WL_OUTPUT_TRANSFORM_NORMAL;
	surface->current->buffer_scale = 1;

	surface->surface_events = surface_events;
	wl_signal_init(&surface->commit);
	wl_resource_set_implementation(resource, &impl, surface, surface_free);
	return surface;
}
