struct linux_surface_synchronization;
struct surface;

void linux_surface_synchronization_new(struct wl_resource *resource, struct
surface *surface);
int linux_surface_synchronization_get_fence(struct linux_surface_synchronization
*self);
void linux_surface_synchronization_send_immediate_release(struct
linux_surface_synchronization *self);
void linux_surface_synchronization_send_fenced_release(struct
linux_surface_synchronization *self, int fence_fd);
