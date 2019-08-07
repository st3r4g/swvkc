#ifndef MYSURFACE_H
#define MYSURFACE_H

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include <util/box.h>

struct surface;
typedef void (*surface_map_t)(struct surface *, void *);
typedef void (*surface_unmap_t)(struct surface *, void *);
typedef void (*surface_contents_update_t)(struct surface *, void *);

struct surface_events {
	surface_map_t map;
	surface_unmap_t unmap;
	surface_contents_update_t contents_update;

	void *user_data;
};

enum role {
	ROLE_NONE,
	ROLE_CURSOR,
	ROLE_DRAG_AND_DROP_ICON,
	ROLE_SUBSURFACE,
	ROLE_FULLSCREEN,      // from fullscreen-shell
	ROLE_XDG_POPUP,       // from xdg-shell
	ROLE_XDG_TOPLEVEL     // from xdg-shell
};

enum base_role {
	BASE_ROLE_NONE,
	BASE_ROLE_XDG_SURFACE // from xdg-shell
};

/*
 * The double-buffered state
 */

struct dbuf_state {
	struct wl_resource *buffer;
/* need this because of how DRM OUT_FENCE_PTR works */
	struct wl_resource *previous_buffer;
	struct box damage;
	struct box buffer_damage;
	struct wl_resource *opaque_region;
	struct wl_resource *input_region;
	enum wl_output_transform buffer_transform;
	int32_t buffer_scale;
};

struct surface {
	struct dbuf_state *current, *pending;

	uint8_t staged; // bitmask

	struct wl_resource *frame;

	enum role role;
	enum base_role base_role;
	void *role_object;
	void *base_role_object;

	bool is_mapped;

	struct wl_signal commit;

	struct surface_events surface_events;
};

struct surface *surface_new(struct wl_resource *resource, struct surface_events
surface_events);

#endif
