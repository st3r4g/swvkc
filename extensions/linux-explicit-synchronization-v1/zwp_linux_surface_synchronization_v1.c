#define _POSIX_C_SOURCE 200809L
#include <wayland-server-core.h>

#include <core/wl_surface.h>
#include <extensions/linux-explicit-synchronization-v1/zwp_linux_surface_synchronization_v1.h>
#include <util/log.h>
#include <util/util.h>

#include <linux-explicit-synchronization-unstable-v1-server-protocol.h>

#include <stdlib.h>
#include <unistd.h>

/*
 * TODO: Protocol errors
 */

enum staged_field {
	FD = 1 << 0
};

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

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	struct linux_surface_synchronization *self
	 = wl_resource_get_user_data(resource);
	if (self->staged & FD)
		close(self->pending->fd);
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
	struct linux_surface_synchronization *self
	 = wl_resource_get_user_data(resource);
	struct wl_resource *buffer_release_resource = wl_resource_create(client,
	&zwp_linux_buffer_release_v1_interface, 1, release);
	self->buffer_release_resource = buffer_release_resource;
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

	self->staged = 0;
}

static void destroyed(struct wl_resource *resource) {
	struct linux_surface_synchronization *self
	 = wl_resource_get_user_data(resource);
/*
 * Unregister this object as an extension to the surface
 */
	struct extension_node *node;
	wl_list_for_each(node, &self->surface->extensions, link)
		if (node->resource == resource)
			break;
	wl_list_remove(&node->link);
	free(node);

	free(self);
}

void linux_surface_synchronization_new(struct wl_resource *resource, struct
surface *surface) {
	struct linux_surface_synchronization *self
	 = calloc(1, sizeof(struct linux_surface_synchronization));
	self->pending = malloc(sizeof(struct dbuf_state_));
	self->pending->fd = -1;
	self->current = malloc(sizeof(struct dbuf_state_));
	self->current->fd = -1;

	self->commit.notify = commit_notify;
	wl_signal_add(&surface->commit, &self->commit);

	self->surface = surface;

	wl_resource_set_implementation(resource, &impl, self, destroyed);
/*
 * Register this object as an extension to the surface
 */
	struct extension_node *node = malloc(sizeof(struct extension_node));
	node->resource = resource;
	wl_list_insert(&surface->extensions, &node->link);
}

int linux_surface_synchronization_get_fence(struct linux_surface_synchronization
*self) {
	return self->current->fd;
}

void linux_surface_synchronization_send_immediate_release(struct
linux_surface_synchronization *self) {
	if (!self || !self->buffer_release_resource)
		return;
	zwp_linux_buffer_release_v1_send_immediate_release(self->buffer_release_resource);
	wl_resource_destroy(self->buffer_release_resource);
	self->buffer_release_resource = NULL;
}

void linux_surface_synchronization_send_fenced_release(struct
linux_surface_synchronization *self, int fence_fd) {
	if (!self || !self->buffer_release_resource || fence_fd < 0)
		return;
	zwp_linux_buffer_release_v1_send_fenced_release(self->buffer_release_resource,
	 fence_fd);
/*
 * Wayland has duped the fd so we can close it right away
 */
	close(fence_fd);
	wl_resource_destroy(self->buffer_release_resource);
	self->buffer_release_resource = NULL;
}
