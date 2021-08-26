#define _POSIX_C_SOURCE 200112L

#include <wayland-server-core.h>

#include <globals.h> // libswvkc-wl
#include <core/keyboard.h>
#include <core/wl_surface.h>
#include <core/wl_pointer.h>
#include <core/subsurface.h>
#include <extensions/linux-dmabuf-unstable-v1/zwp_linux_dmabuf_v1.h>
#include <extensions/linux-explicit-synchronization-v1/zwp_linux_surface_synchronization_v1.h>
#include <extensions/xdg_shell/xdg_surface.h>
#include <extensions/xdg_shell/xdg_toplevel.h>
#include <util/util.h>

#include "gbm_.h"
#include "atomic.h"
#include "modeset.h" // WARNING: probably bad design, try to decouple (if possible)
#include "legacy_wl_drm_.h"

#include <assert.h>
#include <stdlib.h>

static struct wl_display *D;
static struct keyboard * keyboard_g;
static struct surface* surface_g;

void wayland_init() {
	D = wl_display_create();
	assert(D);

	setenv("XDG_RUNTIME_DIR", "/tmp", 0);

	const char *socket = wl_display_add_socket_auto(D);
	assert(socket);

	setenv("WAYLAND_DISPLAY", socket, 0);

	create_globals(D, 1, NULL);
	legacy_wl_drm_init(D, gbm_get_device());
}

void wayland_flush() {
	wl_display_flush_clients(D);
}

int wayland_get_fd() {
	struct wl_event_loop *el = wl_display_get_event_loop(D);
	assert(el);
	return wl_event_loop_get_fd(el);
}

void wayland_read() {
	struct wl_event_loop *el = wl_display_get_event_loop(D);
	assert(el);
	wl_event_loop_dispatch(el, 0);
	// We don't assert its return code because the first time it returns -1...
}

void wayland_send_key(uint32_t key, uint32_t state) {
	if (keyboard_g) keyboard_send_key(keyboard_g, key, state);
	/*keyboard_send_modifiers(data,
	 e->mods_depressed, e->mods_latched,
	  e->mods_locked, e->group);*/
}

static void shmbuf(struct wl_resource *buffer) {
	struct wl_shm_buffer *shm_buffer = wl_shm_buffer_get(buffer);
	uint32_t width = wl_shm_buffer_get_width(shm_buffer);
	uint32_t height = wl_shm_buffer_get_height(shm_buffer);
	uint32_t stride = wl_shm_buffer_get_stride(shm_buffer);
	//uint32_t format = wl_shm_buffer_get_format(shm_buffer);
	uint8_t *data = wl_shm_buffer_get_data(shm_buffer);
	wl_shm_buffer_begin_access(shm_buffer);
	modeset_copy(stride, width, height, data);
	wl_shm_buffer_end_access(shm_buffer);
	wl_buffer_send_release(buffer);
}

static struct wl_resource *to_release[2];

static void dmabuf(struct wl_resource *dmabuf) {
	uint32_t width = wl_buffer_dmabuf_get_width(dmabuf);
	uint32_t height = wl_buffer_dmabuf_get_height(dmabuf);
	uint32_t format = wl_buffer_dmabuf_get_format(dmabuf);
	//uint32_t num_planes = wl_buffer_dmabuf_get_num_planes(dmabuf);
	uint32_t *strides = wl_buffer_dmabuf_get_strides(dmabuf);
	uint32_t *offsets = wl_buffer_dmabuf_get_offsets(dmabuf);
	uint64_t *mods = wl_buffer_dmabuf_get_mods(dmabuf);

	uint32_t *handle_ptr = wl_buffer_dmabuf_get_subsystem_object(dmabuf,
	 SUBSYSTEM_DRM);

	atomic_commit(*handle_ptr);

	if (!to_release[0])
		to_release[0] = dmabuf;
	else if (!to_release[1])
		to_release[1] = dmabuf;
	else
		assert(false);
}

/*
 * Handle events produced by Wayland objects: `object`_`event`_notify
 */

#include <stdio.h>

void surface_map_notify(struct surface *surface, void *user_data) {
	if (surface->role == ROLE_CURSOR)
		return;
	if (surface->role == ROLE_SUBSURFACE)
		return;

	struct wl_array array;
	wl_array_init(&array); //Need the currently pressed keys
	struct wl_client *client = wl_resource_get_client(surface->resource);
	struct wl_resource *keyboard = NULL;
	keyboard = util_wl_client_get_keyboard(client);
	if (keyboard)
		wl_keyboard_send_enter(keyboard, 0, surface->resource, &array);
	struct wl_resource *pointer = NULL;
	pointer = util_wl_client_get_pointer(client);
	if (pointer) {
		wl_pointer_send_enter(pointer, 1, surface->resource,
		 wl_fixed_from_int(0), wl_fixed_from_int(0));
		if (wl_resource_get_version(pointer) >= WL_POINTER_FRAME_SINCE_VERSION)
		wl_pointer_send_frame(pointer);
	}

	surface_g = surface;
}

void surface_unmap_notify(struct surface *surface, void *user_data) {
}

void surface_contents_update_notify(struct surface *surface, void *user_data) {
	struct wl_resource *buffer = surface->current->buffer;
	/*
	 * If the buffer has been detached, do nothing
	 */
	if (!buffer)
		return;

	if (surface->role == ROLE_CURSOR) {
		// TODO
		return;
	}

	if (wl_buffer_is_dmabuf(buffer)) {
		dmabuf(buffer);
	} else {
		shmbuf(buffer);
		// still not vblank driven here
		if (surface && surface->frame) {
			wl_callback_send_done(surface->frame, 0);
			wl_resource_destroy(surface->frame);
			surface->frame = 0;
		}
	}
}

void xdg_toplevel_init_notify(struct xdg_toplevel_data *xdg_toplevel, void
*user_data) {
	struct wl_array array;
	wl_array_init(&array);
	int32_t *state1 = wl_array_add(&array, sizeof(int32_t));
	*state1 = XDG_TOPLEVEL_STATE_ACTIVATED;
	int32_t *state2 = wl_array_add(&array, sizeof(int32_t));
	*state2 = XDG_TOPLEVEL_STATE_MAXIMIZED;
	xdg_toplevel_send_configure(xdg_toplevel->resource, 1920,
	1080, &array);
	xdg_surface_send_configure(xdg_toplevel->xdg_surface_data->self, 0);
}

void buffer_dmabuf_create_notify(struct wl_buffer_dmabuf_data *dmabuf, void
*user_data) {
	uint32_t num_planes = 1;
	int32_t *fds = dmabuf->fds;
	uint32_t width = dmabuf->width;
	uint32_t height = dmabuf->height;
	uint32_t format = dmabuf->format;
	uint32_t *strides = dmabuf->strides;
	uint32_t *offsets = dmabuf->offsets;
	uint64_t *mods = dmabuf->modifiers;

	//struct fb *fb = wl_buffer_dmabuf_get_subsystem_object(dmabuf_resource,
	// SUBSYSTEM_DRM);

	uint32_t handle = gbm_import_from_dmabuf(num_planes, fds, width, height, format, strides, offsets, mods[0]);
	// TODO: migrate to drmPrimeFDToHandle (but refcounting issues?)
	assert(handle);

	uint32_t *handle_ptr = malloc(sizeof(*handle_ptr));
	*handle_ptr = modeset_add_fb(width, height, format, handle, strides[0], offsets[0], mods[0]);
	dmabuf->subsystem_object[SUBSYSTEM_DRM] = handle_ptr;
}

void buffer_dmabuf_destroy_notify(struct wl_buffer_dmabuf_data *dmabuf, void
*user_data) {
	/*uint32_t *handle_ptr = wl_buffer_dmabuf_get_subsystem_object(dmabuf,
	 SUBSYSTEM_DRM);
	free(handle_ptr);*/
	// TODO clean gbm
}

#include "xkb.h"

void keyboard_init_notify(struct keyboard *keyboard, void *user_data) {
	int32_t fd = xkb_get_keymap_fd();
	uint32_t size = xkb_get_keymap_size();
	keyboard_send_keymap(keyboard, fd, size);
	keyboard_g = keyboard; //WARNING
}

void page_flip_handler(int fd, unsigned int sequence, unsigned int
tv_sec, unsigned int tv_usec, void *user_data) {
	if (surface_g && surface_g->frame) {
		uint32_t ms = tv_sec * 1000 + tv_usec / 1000;
		wl_callback_send_done(surface_g->frame, ms);
		wl_resource_destroy(surface_g->frame);
		surface_g->frame = 0;
	}
	if (to_release[1]) {
		wl_buffer_send_release(to_release[1]);
		to_release[1] = NULL;
	}
	if (to_release[0]) {
		to_release[1] = to_release[0];
		to_release[0] = NULL;
	}
}
