#include <wayland-util.h>

struct server {
	struct wl_display *display;

	struct input *input;
	struct screen *screen;
/*
 * Temporary way to manage surfaces, it should evolve into a tree
 */
	struct wl_list mapped_surfaces_list;
	struct wl_list bufres_list; // Buffers that have been scanout
	struct wl_list lss_list;
};
