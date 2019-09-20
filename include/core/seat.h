#include <wayland-server-core.h>
#include <core/keyboard.h>

struct seat {
	struct keyboard_events keyboard_events;
};

struct seat *seat_new(struct wl_resource *resource, struct
keyboard_events keyboard_events);
