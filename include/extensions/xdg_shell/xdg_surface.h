#ifndef MYXDGSURFACE_H
#define MYXDGSURFACE_H

#include <extensions/xdg_shell/xdg_toplevel.h>

#include <wayland-server-core.h>
#include <util/box.h>

struct xdg_surface0;
typedef void (*xdg_surface_contents_update_t)(struct xdg_surface0 *, void *);

typedef void (*callback_t)(void *);

enum map_condition_field {
	MAP_CONDITION_ROLE_ASSIGNED = 1 << 0,
	MAP_CONDITION_BASE_ROLE_INIT_COMMIT = 1 << 1,
	MAP_CONDITION_ROLE_INIT_COMMIT = 1 << 2,
	MAP_CONDITION_BUFFER_COMMIT = 1 << 3,
};

struct xdg_surface_state0 {
	struct box window_geometry;
};

struct xdg_surface0 {
	struct wl_resource *self;
	struct wl_resource *wm_base;
	struct wl_resource *surface;
	struct wl_resource *keyboard;
	struct server *server;
	uint8_t map_conditions;
	struct xdg_surface_state0 *pending, *current;
	struct wl_listener commit;
	struct wl_list link;

	struct wl_signal contents_update;

	xdg_surface_contents_update_t contents_update_callback;
	void *user_data;

	struct xdg_toplevel_events xdg_toplevel_events;

	callback_t child_destroy_notify;
	void *data;
};

struct xdg_surface0 *xdg_surface_new
(
	struct wl_resource *resource,
	struct wl_resource *surface_resource,
	struct wl_resource *wm_base,
	struct server *server,
	xdg_surface_contents_update_t xdg_surface_contents_update,
	void *user_data,
	struct xdg_toplevel_events xdg_toplevel_events,
	callback_t child_destroy_notify,
	void *data
);
void xdg_surface_map_condition_satisfied
(
	struct xdg_surface0 *xdg_surface,
	enum map_condition_field condition
);
bool xdg_surface_map_condition_check(struct xdg_surface0 *xdg_surface, enum
map_condition_field condition);

#endif
