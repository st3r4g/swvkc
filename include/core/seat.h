#include <wayland-server-core.h>

struct seat {
	struct input *input;
	struct keyboard *keyb;

	struct wl_list link;
};

struct seat *seat_new(struct wl_resource *resource, struct input *input);
