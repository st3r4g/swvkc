#define _POSIX_C_SOURCE 200809L
#include <wayland-server-core.h>

#include <core/wl_surface.h>
#include <extensions/linux-explicit-synchronization-v1/zwp_linux_surface_synchronization_v1.h>
#include <extensions/linux-explicit-synchronization-v1/zwp_linux_buffer_release_v1.h>

#include <linux-explicit-synchronization-unstable-v1-server-protocol.h>

#include <stdlib.h>
#include <unistd.h>

enum staged_field {
	FD = 1 << 0
};

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void set_acquire_fence(struct wl_client *client, struct wl_resource
*resource, int32_t fd) {
	struct linux_surface_synchronization *self
	 = wl_resource_get_user_data(resource);
	self->pending->fd = fd;
	self->staged |= FD;
}

static void get_release(struct wl_client *client, struct wl_resource *resource,
uint32_t release) {
	struct wl_resource *child = wl_resource_create(client,
	&zwp_linux_buffer_release_v1_interface, 1, release);
	linux_buffer_release_new(child);
}

static const struct zwp_linux_surface_synchronization_v1_interface impl = {
	.destroy = destroy,
	.set_acquire_fence = set_acquire_fence,
	.get_release = get_release
};

static void commit_notify(struct wl_listener *listener, void *data) {
	struct linux_surface_synchronization *self;
	self = wl_container_of(listener, self, commit);

	if (self->staged & FD)
		self->current->fd = self->pending->fd;

	close(self->current->fd); //XXX simulate importing
}

static void destroyed(struct wl_resource *resource) {
	struct linux_surface_synchronization *self
	 = wl_resource_get_user_data(resource);
	free(self);
}

struct linux_surface_synchronization *linux_surface_synchronization_new(struct
wl_resource *resource, struct surface *surface) {
	struct linux_surface_synchronization *self
	 = calloc(1, sizeof(struct linux_surface_synchronization));
	self->pending = malloc(sizeof(struct dbuf_state_));
	self->pending->fd = -1;
	self->current = malloc(sizeof(struct dbuf_state_));
	self->current->fd = -1;

	self->commit.notify = commit_notify;
	wl_signal_add(&surface->commit, &self->commit);

	wl_resource_set_implementation(resource, &impl, self, destroyed);
	return self;
}
