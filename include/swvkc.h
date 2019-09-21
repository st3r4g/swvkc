#include <wayland-util.h>
#include <stdbool.h>

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

void vblank_notify(int gpu_fd, unsigned int sequence, unsigned int
tv_sec, unsigned int tv_usec, void *user_data, bool vblank_has_page_flip);
int gpu_ev_handler(int fd, uint32_t mask, void *data);
int key_ev_handler(int key_fd, uint32_t mask, void *data);
