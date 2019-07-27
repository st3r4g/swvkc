#ifndef MYSURFACE_H
#define MYSURFACE_H

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include <util/box.h>

/*enum role {
	ROLE_NONE,
	ROLE_CURSOR,
	ROLE_DRAG_AND_DROP_ICON,
	ROLE_SUBSURFACE,
	ROLE_XDG_POPUP,
	ROLE_XDG_TOPLEVEL
}

enum {
	BASE_ROLE_XDG_SURFACE

}*/

/*
 * The double-buffered state
 */

struct dbuf_state {
	struct wl_resource *buffer;
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

	struct server *server;
	struct texture *texture;

	struct wl_resource *frame;

//	struct wl_resource *role_object;

	struct wl_signal commit;
};

struct surface *surface_new(struct wl_resource *resource, struct server *server);

#endif
