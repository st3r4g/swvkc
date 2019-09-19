struct dbuf_state_ {
	int fd;
};

struct linux_surface_synchronization {
	struct dbuf_state_ *current, *pending;
	uint8_t staged; // bitmask
	struct wl_listener commit;
};

struct surface;

struct linux_surface_synchronization *linux_surface_synchronization_new(struct
wl_resource *resource, struct surface *surface);
