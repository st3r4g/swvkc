struct surface;

struct dbuf_state_ {
	int fd;
};

struct linux_surface_synchronization {
	struct dbuf_state_ *current, *pending;
	uint8_t staged; // bitmask
	struct wl_listener commit;

	struct surface *surface;
	struct wl_resource *buffer_release_resource;
};

struct linux_surface_synchronization *linux_surface_synchronization_new(struct
wl_resource *resource, struct surface *surface);
void linux_surface_synchronization_send_immediate_release(struct
linux_surface_synchronization *self);
void linux_surface_synchronization_send_fenced_release(struct
linux_surface_synchronization *self, int fence_fd);
