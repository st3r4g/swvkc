#include <core/wl_surface.h>

struct compositor {
	struct surface_events surface_events;
};

struct compositor *compositor_new(struct wl_resource *resource, struct
surface_events surface_events);
