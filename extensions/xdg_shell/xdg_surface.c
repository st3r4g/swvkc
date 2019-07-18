#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>

#include <wayland-server-protocol.h> // Just for one event
#include <xdg-shell-server-protocol.h>
#include <core/wl_surface.h>
#include <extensions/xdg_shell/xdg_surface.h>
#include <extensions/xdg_shell/xdg_toplevel.h>
#include <extensions/linux-dmabuf-unstable-v1/wl_buffer_dmabuf.h>
#include <util/log.h>
#include <util/util.h>
#include <backend/screen.h>
#include <backend/vulkan.h>
#include <server.h>

/*static enum wl_iterator_result keyboard_set(struct wl_resource *resource,
void *user_data) {
	if (!strcmp(wl_resource_get_class(resource), "wl_keyboard")) {
		struct xdg_surface0 *xdg_surface = user_data;
		xdg_surface->keyboard = resource;
		return WL_ITERATOR_STOP;
	}
	return WL_ITERATOR_CONTINUE;
}*/

static void destroy(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void get_toplevel(struct wl_client *client, struct wl_resource *resource,
uint32_t id) {
	struct xdg_surface0 *data = wl_resource_get_user_data(resource);
	struct wl_resource *toplevel_resource = wl_resource_create(client,
	&xdg_toplevel_interface, 1, id);
	xdg_toplevel_new(toplevel_resource, data, data->server);
	xdg_surface_send_configure(resource, 0);
}

static void get_popup(struct wl_client *client, struct wl_resource *resource,
uint32_t id, struct wl_resource *parent, struct wl_resource *positioner) {

}

static void set_window_geometry(struct wl_client *client, struct wl_resource
*resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	struct xdg_surface0 *xdg_surface = wl_resource_get_user_data(resource);
	xdg_surface->pending->window_geometry.x = x;
	xdg_surface->pending->window_geometry.y = y;
	xdg_surface->pending->window_geometry.width = width;
	xdg_surface->pending->window_geometry.height = height;

}

static void ack_configure(struct wl_client *client, struct wl_resource
*resource, uint32_t serial) {
	struct xdg_surface0 *xdg_surface = wl_resource_get_user_data(resource);
	struct wl_array array;
	wl_array_init(&array); //Need the currently pressed keys
	xdg_surface->keyboard = util_wl_client_get_keyboard(client);
//	wl_client_for_each_resource(client, keyboard_set, xdg_surface);
	if (xdg_surface->keyboard) //TODO: not sure this is the right place
	wl_keyboard_send_enter(xdg_surface->keyboard, 0, xdg_surface->surface,
	&array);
}

static const struct xdg_surface_interface impl = {
	.destroy = destroy,
	.get_toplevel = get_toplevel,
	.get_popup = get_popup,
	.set_window_geometry = set_window_geometry,
	.ack_configure = ack_configure
};

void dmabuf(struct wl_resource *buffer) {
	uint32_t width = wl_buffer_dmabuf_get_width(buffer);
	uint32_t height = wl_buffer_dmabuf_get_height(buffer);
	uint32_t format = wl_buffer_dmabuf_get_format(buffer);
	int fd = wl_buffer_dmabuf_get_fd(buffer);
	int stride = wl_buffer_dmabuf_get_stride(buffer);
	int offset = wl_buffer_dmabuf_get_offset(buffer);
	uint64_t mod = wl_buffer_dmabuf_get_mod(buffer);
errlog("%d %d %d %d %d %d %d", width, height, format, fd, stride, offset, mod);
	// 1) TODO Copy window buffer to screen
	// 2) schedule pageflip with new screen content
	/*screen_post_direct(server_get_screen(surface->server),
	width, height, format, fd, stride, offset, mod);*/
	vulkan_main(1, fd, width, height, stride, mod);
}

void shmbuf(struct wl_resource *buffer) {
	struct wl_shm_buffer *shm_buffer = wl_shm_buffer_get(buffer);
	uint32_t width = wl_shm_buffer_get_width(shm_buffer);
	uint32_t height = wl_shm_buffer_get_height(shm_buffer);
	uint32_t stride = wl_shm_buffer_get_stride(shm_buffer);
	uint32_t format = wl_shm_buffer_get_format(shm_buffer);
	uint8_t *data = wl_shm_buffer_get_data(shm_buffer);
	errlog("BEGIN ACCESS");
	wl_shm_buffer_begin_access(shm_buffer);
	vulkan_render_shm_buffer(width, height, stride, format, data);
	wl_shm_buffer_end_access(shm_buffer);
	errlog("END ACCESS");
}

static void commit_notify(struct wl_listener *listener, void *data) {
	struct xdg_surface0 *xdg_surface;
	xdg_surface = wl_container_of(listener, xdg_surface, commit);
	struct xdg_surface_state0 *pending = xdg_surface->pending, *current =
	xdg_surface->current;

	current->window_geometry.x = pending->window_geometry.x;
	current->window_geometry.y = pending->window_geometry.y;
	current->window_geometry.width = pending->window_geometry.width;
	current->window_geometry.height = pending->window_geometry.height;
	
	struct surface *surface = data;
		if (surface->current->buffer && surface->staged & (1 << 0) && server_surface_is_focused(xdg_surface)) {
			struct wl_resource *buffer = surface->current->buffer;
			if (wl_buffer_is_dmabuf(buffer))
				dmabuf(buffer);
			else
				shmbuf(buffer);
			screen_post(server_get_screen(surface->server));
//			NEEDED, screen refresh (scanout) happens automatically
//			from the currently bound buffer. Will be needed for
//			double buffering
			wl_buffer_send_release(buffer);
		} else if (surface->current->buffer && surface->staged & (1 << 0)){
			struct wl_resource *buffer = surface->current->buffer;
			wl_buffer_send_release(buffer);
//			surface->texture = 0, buf_w = 0, buf_h = 0;
		}
	
}

static void xdg_surface_free(struct wl_resource *resource) {
	struct xdg_surface0 *xdg_surface = wl_resource_get_user_data(resource);
	free(xdg_surface->current);
	free(xdg_surface->pending);
	free(xdg_surface);
}

struct xdg_surface0 *xdg_surface_new(struct wl_resource *resource, struct
wl_resource *surface_resource, struct server *server) {
	struct surface *surface = wl_resource_get_user_data(surface_resource);
	struct xdg_surface0 *xdg_surface = calloc(1, sizeof(struct
	xdg_surface0));
	xdg_surface->self = resource;
	xdg_surface->server = server;
	xdg_surface->pending = calloc(1, sizeof(struct xdg_surface_state0));
	xdg_surface->current = calloc(1, sizeof(struct xdg_surface_state0));
	xdg_surface->surface = surface_resource;

	xdg_surface->commit.notify = commit_notify;
	wl_signal_add(&surface->commit, &xdg_surface->commit);
	wl_resource_set_implementation(resource, &impl, xdg_surface,
	xdg_surface_free);
	return xdg_surface;
}
