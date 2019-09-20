#define _POSIX_C_SOURCE 200809L

#include <core/subsurface.h>
#include <core/wl_surface.h>
#include <util/log.h>

#include <wayland-server-protocol.h>

#include <stdlib.h>

enum staged_field {
	PARENT = 1 << 0,
};

static void destroy(struct wl_client *client, struct wl_resource *resource) {

}

static void set_position(struct wl_client *client, struct wl_resource *resource,
int32_t x, int32_t y) {

}

static void place_above(struct wl_client *client, struct wl_resource *resource,
struct wl_resource *sibling) {

}

static void place_below(struct wl_client *client, struct wl_resource *resource,
struct wl_resource *sibling) {

}

static void set_sync(struct wl_client *client, struct wl_resource *resource) {

}

static void set_desync(struct wl_client *client, struct wl_resource *resource) {

}

static const struct wl_subsurface_interface impl = {
	.destroy = destroy,
	.set_position = set_position,
	.place_above = place_above,
	.place_below = place_below,
	.set_sync = set_sync,
	.set_desync = set_desync,
};

enum map_condition_field {
	MAP_CONDITION_BUFFER_COMMIT = 1 << 0,
	MAP_CONDITION_PARENT_MAPPED = 1 << 1,
};

bool subsurface_is_mapped(struct subsurface *subsurface) {
	static const uint8_t all_conditions_set =
	 MAP_CONDITION_BUFFER_COMMIT |
	 MAP_CONDITION_PARENT_MAPPED;
	return subsurface->map_conditions == all_conditions_set;
}

bool subsurface_map_condition_check(struct subsurface *subsurface, enum
map_condition_field condition) {
	return subsurface->map_conditions & condition;
}

void subsurface_map_condition_satisfied(struct subsurface *subsurface, enum
map_condition_field condition) {
	subsurface->map_conditions |= condition;
	if (subsurface_is_mapped(subsurface)) {
		struct surface *surface = wl_resource_get_user_data(subsurface->surface);
		// TODO: lump the following lines in a func in surface.c
		void *user_data = surface->surface_events.user_data;
		surface->surface_events.map(surface, user_data);
		surface->is_mapped = true;
	}
}

static void commit_notify(struct wl_listener *listener, void *data) {
	errlog("subsurface_commit_notify");
	struct subsurface *subsurface;
	subsurface = wl_container_of(listener, subsurface, commit);
	struct surface *surface = data;
	if (!subsurface_map_condition_check(subsurface,
	 MAP_CONDITION_BUFFER_COMMIT) && surface->current->buffer) {
		subsurface_map_condition_satisfied(subsurface,
		                                  MAP_CONDITION_BUFFER_COMMIT);
	}
}

static void parent_commit_notify(struct wl_listener *listener, void *data) {
	errlog("subsurface_parent_commit_notify");
	struct subsurface *subsurface;
	subsurface = wl_container_of(listener, subsurface, parent_commit);

	struct subsurface_dbuf_state *current = subsurface->current;
	struct subsurface_dbuf_state *pending = subsurface->pending;

	if (subsurface->staged & PARENT)
		current->parent = pending->parent;

	subsurface->staged &= ~PARENT;
	
	struct surface *parent = wl_resource_get_user_data(current->parent);
	if (parent->is_mapped)
		subsurface_map_condition_satisfied(subsurface,
		                                  MAP_CONDITION_PARENT_MAPPED);
	// TODO: else listen for parent map signal
	
	wl_list_remove(&subsurface->parent_commit.link);
}

static void parent_map_notify(struct wl_listener *listener, void *data) {
	// TODO: let's hope that the parent comes already mapped for now
	// Will have to add a surface.map signal
}

static void subsurface_free(struct wl_resource *resource) {
	struct subsurface *subsurface = wl_resource_get_user_data(resource);
	free(subsurface->current);
	free(subsurface->pending);
	free(subsurface);
}

struct subsurface *wl_subsurface_new(struct wl_resource *subsurface_resource, struct
wl_resource *surface_resource, struct wl_resource *parent_resource) {
	errlog("subsurface_new");
	struct surface *surface = wl_resource_get_user_data(surface_resource);
	struct surface *parent = wl_resource_get_user_data(parent_resource);

	struct subsurface *subsurface = calloc(1, sizeof(struct subsurface));
	subsurface->surface = surface_resource;

	subsurface->pending = calloc(1, sizeof(struct subsurface_dbuf_state));
	subsurface->pending->parent = parent_resource;
	subsurface->staged |= PARENT;

	subsurface->current = calloc(1, sizeof(struct subsurface_dbuf_state));

	subsurface->commit.notify = commit_notify;
	wl_signal_add(&surface->commit, &subsurface->commit);
	subsurface->parent_commit.notify = parent_commit_notify;
	wl_signal_add(&parent->commit, &subsurface->parent_commit);

	wl_resource_set_implementation(subsurface_resource, &impl, subsurface,
	 subsurface_free);
	return subsurface;
}
