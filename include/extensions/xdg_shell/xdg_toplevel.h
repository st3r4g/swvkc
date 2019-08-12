#ifndef MYXDGTOPLEVEL_H
#define MYXDGTOPLEVEL_H

struct xdg_toplevel_data;

typedef void (*xdg_toplevel_init_t)(struct xdg_toplevel_data *, void *);

struct xdg_toplevel_events {
	xdg_toplevel_init_t init;

	void *user_data;
};

struct xdg_toplevel_data {
	struct wl_resource *resource;
	struct xdg_surface0 *xdg_surface_data;
	struct wl_listener commit;
	char *app_id;

	struct xdg_toplevel_events events;
};

struct xdg_toplevel_data *xdg_toplevel_new(struct wl_resource *, struct
xdg_surface0 *, struct xdg_toplevel_events events);

char *xdg_toplevel_get_app_id(struct xdg_toplevel_data *);

#endif
