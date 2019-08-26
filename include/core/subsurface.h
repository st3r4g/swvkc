#include <wayland-server-core.h>

#include <stdint.h>

/*
 * The double-buffered state
 */

struct subsurface_dbuf_state {
	struct wl_resource *parent;
};

struct subsurface {
	struct subsurface_dbuf_state *current, *pending;
	uint8_t staged; // bitmask

	struct wl_resource *surface;

	uint8_t map_conditions; // bitmask

	struct wl_listener commit;
	struct wl_listener parent_commit;
};

struct subsurface *wl_subsurface_new(struct wl_resource *subsurface_resource, struct
wl_resource *surface_resource, struct wl_resource *parent_resource);
