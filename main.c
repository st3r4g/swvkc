#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <xdg-shell-server-protocol.h>
//#include <xdg-shell-unstable-v6-server-protocol.h>
#include <linux-dmabuf-unstable-v1-server-protocol.h>
#include <fullscreen-shell-unstable-v1-server-protocol.h>

#include <legacy_wl_drm.h> // legacy
#include <backend/input.h>
#include <backend/screen.h>
#include <backend/vulkan.h>
#include <util/log.h>
#include <core/compositor.h>
#include <core/data_device_manager.h>
#include <core/output.h>
#include <core/seat.h>
#include <core/keyboard.h>
#include <core/wl_subcompositor.h>
#include <core/wl_surface.h> // da togliere
#include <extensions/xdg_shell/xdg_wm_base.h>
#include <extensions/xdg_shell/xdg_surface.h>
#include <extensions/xdg_shell/xdg_toplevel.h>
#include <extensions/linux-dmabuf-unstable-v1/zwp_linux_dmabuf_v1.h>
#include <extensions/linux-dmabuf-unstable-v1/wl_buffer_dmabuf.h>
#include <extensions/fullscreen-shell-unstable-v1/zwp_fullscreen_shell_v1.h>
#include <util/box.h>
#include <util/util.h>

#include <backend/vulkan.h>

#include <linux/input-event-codes.h>

struct server {
	struct wl_display *display;

	struct input *input;
	struct screen *screen;

//	struct wl_list xdg_shell_v6_list;
	struct wl_list seat_list;

	struct wl_list xdg_surface_list;
/*
 * Temporary way to manage surfaces, it should evolve into a tree
 */
	struct wl_list mapped_surfaces_list;
	struct wl_list bufres_list; // Buffers that have been scanout

	struct wl_event_source *event;
};

struct surface_node {
	struct surface *surface;
	struct wl_list link;
};

struct bufres_node {
	struct wl_resource *bufres;
	struct wl_list link;
};

static int out_fence_handler(int fd, uint32_t mask, void *data);

struct surface *focused_surface(struct server *server) {
	struct surface_node *node;
	node = wl_container_of(server->mapped_surfaces_list.next, node, link);
	return node->surface;
}

void dmabuf(struct wl_resource *dmabuf_resource, struct screen *screen) {
	uint32_t width = wl_buffer_dmabuf_get_width(dmabuf_resource);
	uint32_t height = wl_buffer_dmabuf_get_height(dmabuf_resource);
/*	uint32_t format = wl_buffer_dmabuf_get_format(dmabuf);
	uint32_t num_planes = wl_buffer_dmabuf_get_num_planes(dmabuf);
	uint32_t *strides = wl_buffer_dmabuf_get_strides(dmabuf);
	uint32_t *offsets = wl_buffer_dmabuf_get_offsets(dmabuf);
	uint64_t *mods = wl_buffer_dmabuf_get_mods(dmabuf);*/
	// 1) TODO Copy window buffer to screen
	// 2) schedule pageflip with new screen content
	/*screen_post_direct(server_get_screen(surface->server),
	width, height, format, fd, stride, offset, mod);*/
	struct fb *fb = wl_buffer_dmabuf_get_subsystem_object(dmabuf_resource,
	 SUBSYSTEM_DRM);

	if (screen_is_overlay_supported(screen))
		client_buffer_on_overlay(screen, fb, width, height);
//	else
//		vulkan_main(1, fds[0], width, height, strides[0], mods[0]);
}

void shmbuf(struct wl_resource *buffer) {
	struct wl_shm_buffer *shm_buffer = wl_shm_buffer_get(buffer);
	uint32_t width = wl_shm_buffer_get_width(shm_buffer);
	uint32_t height = wl_shm_buffer_get_height(shm_buffer);
	uint32_t stride = wl_shm_buffer_get_stride(shm_buffer);
	uint32_t format = wl_shm_buffer_get_format(shm_buffer);
	uint8_t *data = wl_shm_buffer_get_data(shm_buffer);
	wl_shm_buffer_begin_access(shm_buffer);
	vulkan_render_shm_buffer(width, height, stride, format, data);
	wl_shm_buffer_end_access(shm_buffer);
	wl_buffer_send_release(buffer);
}

/*
 * Handle events produced by Wayland objects: `object`_`event`_notify
 */

void surface_map_notify(struct surface *surface, void *user_data) {
	struct server *server = user_data;
	if (!wl_list_empty(&server->mapped_surfaces_list)) {
		struct surface *old = focused_surface(server);
		struct xdg_surface0 *x = old->base_role_object;
		wl_keyboard_send_leave(x->keyboard, 0, x->surface);
	}
	errlog("A surface has been mapped");
	struct surface_node *node = malloc(sizeof(struct surface_node));
	node->surface = surface;
	wl_list_insert(&server->mapped_surfaces_list, &node->link);
}

void surface_unmap_notify(struct surface *surface, void *user_data) {
	struct server *server = user_data;
	errlog("A surface has been unmapped");
	/*
	 * Is there a way to remove it without going through the list?
	 */
	struct surface_node *node;
	wl_list_for_each(node, &server->mapped_surfaces_list, link)
		if (node->surface == surface)
			break;
	wl_list_remove(&node->link);
	free(node);
	if (!wl_list_empty(&server->mapped_surfaces_list)) {
		struct wl_array array;
		wl_array_init(&array); //Need the currently pressed keys
		struct surface *new = focused_surface(server);
		struct xdg_surface0 *x = new->base_role_object;
		wl_keyboard_send_enter(x->keyboard, 0, x->surface, &array);
	}
}

void surface_contents_update_notify(struct surface *surface, void *user_data) {
	struct server *server = user_data;
/*
 * At this point `page_flip_is_pending()` should always be false (unless the
 * client commits more than one buffer update per signaled frame on purpose)
 */
	if (screen_page_flip_is_pending(server->screen))
		errlog("ERROR: buffer already committed");
	else if (surface->current->buffer && surface == focused_surface(server)) {
		struct wl_resource *buffer = surface->current->buffer;
		struct screen *screen = server->screen;
		if (wl_buffer_is_dmabuf(buffer)) {
			dmabuf(buffer, screen);
			struct bufres_node *node = malloc(sizeof(*node));
			node->bufres = buffer;
			wl_list_insert(&server->bufres_list, &node->link);
		}
		else {
			shmbuf(buffer);
			screen_main(screen);
		}

		int out_fence_fd = screen_atomic_commit(screen);
		if (out_fence_fd < 0)
			return;

		struct wl_event_loop *el =
		 wl_display_get_event_loop(server->display);
		server->event = wl_event_loop_add_fd(el, out_fence_fd,
		 WL_EVENT_READABLE, out_fence_handler, server);
		if (!server->event) // was NULL once (kitty)
			errlog("wl_event_loop_add_fd failed (fd %d)", out_fence_fd);
	}
}

void xdg_toplevel_init_notify(struct xdg_toplevel_data *xdg_toplevel, void
*user_data) {
	struct server *server = user_data;
	struct wl_array array;
	wl_array_init(&array);
	int32_t *state1 = wl_array_add(&array, sizeof(int32_t));
	*state1 = XDG_TOPLEVEL_STATE_ACTIVATED;
	int32_t *state2 = wl_array_add(&array, sizeof(int32_t));
	*state2 = XDG_TOPLEVEL_STATE_MAXIMIZED;
	struct box screen_size = screen_get_dimensions(server->screen);
	xdg_toplevel_send_configure(xdg_toplevel->resource, screen_size.width,
	screen_size.height, &array);
	xdg_surface_send_configure(xdg_toplevel->xdg_surface_data->self, 0);
}

void buffer_dmabuf_create_notify(struct wl_buffer_dmabuf_data *dmabuf, void
*user_data) {
	struct server *server = user_data;
	dmabuf->subsystem_object[SUBSYSTEM_DRM] = screen_fb_create_from_dmabuf(
	 server->screen, dmabuf->width, dmabuf->height, dmabuf->format,
	  dmabuf->num_planes, dmabuf->fds, dmabuf->offsets, dmabuf->strides,
	   dmabuf->modifiers);
/*	dmabuf->subsystem_object[SUBSYSTEM_VULKAN] = renderer_image_from_dmabuf(
	 server->renderer, ...); */
}

void buffer_dmabuf_destroy_notify(struct wl_buffer_dmabuf_data *dmabuf, void
*user_data) {
	struct server *server = user_data;
	struct fb *fb = dmabuf->subsystem_object[SUBSYSTEM_DRM];
	screen_fb_schedule_destroy(server->screen, fb);
/*	void *image = dmabuf->subsystem_object[SUBSYSTEM_VULKAN];
	renderer_image_destroy(server->renderer, image); */

	struct bufres_node *node;
	wl_list_for_each(node, &server->bufres_list, link)
		if (wl_resource_get_user_data(node->bufres) == dmabuf) {
		wl_list_remove(&node->link);
		free(node);
		break;
	}
}

struct surface_node *match_app_id(struct server *server, char *name) {
	struct surface_node *node, *match;
	int i = 0;
	wl_list_for_each(node, &server->mapped_surfaces_list, link) {
		if (node->surface->role != ROLE_XDG_TOPLEVEL)
			continue;
		struct xdg_toplevel_data *data = node->surface->role_object;
		if (!strncasecmp(name, data->app_id, strlen(name)))
			match = node, i++;
	}
	if (i == 1)
		return match;
	else
		return NULL;
}

void server_window_create(struct xdg_toplevel_data *new) {
/*	if (!wl_list_empty(&window_list)) {
		struct xdg_toplevel_data *old;
		old = wl_container_of(window_list.next, old, link);
		wl_keyboard_send_leave(old->xdg_surface_data->keyboard, 0,
		old->xdg_surface_data->surface);
		focused = NULL;
	}
	wl_list_insert(&window_list, &new->link);*/
}

void server_window_destroy(struct xdg_toplevel_data *old) {
/*	struct wl_array array;
	wl_array_init(&array); //Need the currently pressed keys
//	wl_keyboard_send_leave(resource, 0, xdg_surface->surface);
	wl_list_remove(&old->link);
	if (!wl_list_empty(&window_list)) {
		struct xdg_toplevel_data *new;
		new = wl_container_of(window_list.next, new, link);
		wl_keyboard_send_enter(new->xdg_surface_data->keyboard, 0,
		new->xdg_surface_data->surface, &array);
		focused = new;
	} else {
		focused = NULL;
	}*/
}

void server_set_focus(struct xdg_toplevel_data *data) {
}

void server_change_focus(struct server *self, struct surface_node *node) {
/*	errlog("focused window: %s", focused->app_id);
	errlog("new window: %s", data->app_id);
	struct xdg_surface0 *old = focused->xdg_surface_data;
	if (old->self)
		wl_keyboard_send_leave(old->keyboard, 0, old->surface);
	struct wl_array array;
	wl_array_init(&array); //Need the currently pressed keys
	struct xdg_surface0 *new = data->xdg_surface_data;
	if (new->self)
		wl_keyboard_send_enter(new->keyboard, 0, new->surface, &array);
	focused = data;*/
	struct surface *focused = focused_surface(self);
	struct xdg_surface0 *x = focused->base_role_object;
	wl_keyboard_send_leave(x->keyboard, 0, x->surface);
	wl_list_remove(&node->link);
	wl_list_insert(&self->mapped_surfaces_list, &node->link);
	struct wl_array array;
	wl_array_init(&array); //Need the currently pressed keys
	struct xdg_surface0 *x2 = node->surface->base_role_object;
	wl_keyboard_send_enter(x2->keyboard, 0, x2->surface, &array);
}

static void vblank_notify(int gpu_fd, unsigned int sequence, unsigned int
tv_sec, unsigned int tv_usec, void *user_data, bool vblank_has_page_flip) {
	struct server *server = user_data;
//	errlog("VBLANK");
/*
 * Sometimes a page-flip request won't make it to the immediately following
 * vblank due to being issued too close to it. When this happens, we don't want
 * to tell the client to render a new frame. This is the worst case scenario for
 * input lag (= a little more than the inverse of the refresh rate)
 */
	bool pending_page_flip = screen_page_flip_is_pending(server->screen);
	bool skip_frame = pending_page_flip && !vblank_has_page_flip;

	if (!wl_list_empty(&server->mapped_surfaces_list) && !skip_frame) {
		struct surface *surface = focused_surface(server);
		if (surface->frame) {
			uint32_t ms = tv_sec * 1000 + tv_usec / 1000;
			wl_callback_send_done(surface->frame, ms);
			wl_resource_destroy(surface->frame);
			surface->frame = 0;
		}
	}
}

static int out_fence_handler(int fd, uint32_t mask, void *data) {
	struct server *server = data;
//	errlog("SCANOUT COMPLETED");
//	if (server->event)
		wl_event_source_remove(server->event); // sometimes fails with mpv (?)
	close(fd);

/*	if (!wl_list_empty(&server->mapped_surfaces_list)) {
		struct surface *surface = focused_surface(server);
		if (surface->current->previous_buffer)
			wl_buffer_send_release(surface->current->previous_buffer);
	}*/

	if (wl_list_length(&server->bufres_list) > 1) {
		struct bufres_node *node;
		node = wl_container_of(server->bufres_list.prev, node, link);
		wl_buffer_send_release(node->bufres);
		wl_list_remove(&node->link);
		free(node);
	}

	return 0;
}

static int gpu_ev_handler(int fd, uint32_t mask, void *data) {
	drm_handle_event(fd);

	return 0;
}

static int key_ev_handler(int key_fd, uint32_t mask, void *data) {
	static bool steal = false;
	static int i = 0;
	static char name[64] = {'\0'};
	struct server *server = data;
	struct aaa aaa;
	if (input_handle_event(server->input, &aaa)) {
		if (aaa.key == 59) { //F1
			wl_display_terminate(server->display);
			return 0;
		} else if (aaa.key == KEY_LEFTMETA && aaa.state == 1) {
			steal = true;
			memset(name, '\0', i);
			i = 0;
			errlog("Win key pressed");
			return 0;
		} else if (aaa.key == KEY_LEFTMETA && aaa.state == 0) {
			errlog("Win key released, written %s", name);
			steal = false;
			return 0;
		} else if (steal && aaa.state == 1) {
			errlog("the key '%s' was pressed", aaa.name);
			name[i] = aaa.name[0];
			i++;
			struct surface_node *match = match_app_id(server, name);
			errlog("match %p", (void*)match);
			if (match)
				server_change_focus(server, match);
		} else if (!wl_list_empty(&server->mapped_surfaces_list)) {
			struct surface *surface = focused_surface(server);
			struct xdg_surface0 *xdg_surface = surface->base_role_object;
			struct wl_resource *resource = xdg_surface->keyboard;
			if (resource) {
				struct keyboard *data = wl_resource_get_user_data(resource);
				if (data)
				keyboard_send(data, &aaa);
			}
		}
	}
	return 0;
}

static void compositor_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&wl_compositor_interface, version, id);
	struct surface_events surface_events = {
		.map = surface_map_notify,
		.unmap = surface_unmap_notify,
		.contents_update = surface_contents_update_notify,
		.user_data = server
	};
	compositor_new(resource, surface_events);
}

static void data_device_manager_bind(struct wl_client *client, void *data,
uint32_t version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_compositor_interface, version, id);
	data_device_manager_new(resource);
}


static void seat_bind(struct wl_client *client, void *data, uint32_t version,
uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&wl_seat_interface, version, id);
	struct seat *seat = seat_new(resource, server->input);
	wl_list_insert(&server->seat_list, &seat->link);
}

static void subcompositor_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_subcompositor_interface, version, id);
	wl_subcompositor_new(resource);
}

static void output_bind(struct wl_client *client, void *data, uint32_t version,
uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_output_interface, version, id);
	output_new(resource);
}

static void xdg_wm_base_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&xdg_wm_base_interface, version, id);
	struct xdg_toplevel_events xdg_toplevel_events = {
		.init = xdg_toplevel_init_notify,
		.user_data = server
	};
	xdg_wm_base_new(resource, server, xdg_toplevel_events);
}

/*static void xdg_shell_v6_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&zxdg_shell_v6_interface, version, id);
	struct xdg_shell *xdg_shell = xdg_shell_new(resource, server);
	wl_list_insert(&server->xdg_shell_v6_list, &xdg_shell->link);
}*/

static void zwp_linux_dmabuf_v1_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&zwp_linux_dmabuf_v1_interface, version, id);
	struct buffer_dmabuf_events buffer_dmabuf_events = {
		.create = buffer_dmabuf_create_notify,
		.destroy = buffer_dmabuf_destroy_notify,
		.user_data = server
	};
	zwp_linux_dmabuf_v1_new(resource, buffer_dmabuf_events);
}

static void zwp_fullscreen_shell_v1_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&zwp_fullscreen_shell_v1_interface, version, id);
	zwp_fullscreen_shell_v1_new(resource);
}

// For debugging
static bool global_filter(const struct wl_client *client, const struct wl_global
*global, void *data) {
/*	char *client_name = get_a_name((struct wl_client*)client);
	bool condition = wl_global_get_interface(global) == &zwp_fullscreen_shell_v1_interface;
	if (!strcmp(client_name, "weston-simple-d") && condition) {
		free(client_name);
		return false;
	}
	free(client_name);*/
	return true;
}

int main(int argc, char *argv[]) {
	struct server *server = calloc(1, sizeof(struct server));
	wl_list_init(&server->seat_list);
	wl_list_init(&server->xdg_surface_list);
	wl_list_init(&server->mapped_surfaces_list);
	wl_list_init(&server->bufres_list);
//	wl_list_init(&server->xdg_shell_v6_list);

//	wl_list_init(&window_list);

	bool dmabuf = false, dmabuf_mod = false;
	vulkan_init(&dmabuf, &dmabuf_mod);
	const char *words[] = {"DISABLED", "enabled"};
	errlog("swvkc DMABUF support: %s", words[dmabuf]);
	errlog("swvkc DMABUF with MODIFIERS support: %s", words[dmabuf_mod]);

	server->screen = screen_setup(vblank_notify, server, dmabuf_mod);
	if (!server->screen)
		return EXIT_FAILURE;

	vulkan_create_screen_image(screen_get_fb_buffer(server->screen));

	server->display = wl_display_create();
	struct wl_display *D = server->display;
	setenv("WAYLAND_DISPLAY", wl_display_add_socket_auto(D), 0);

	wl_global_create(D, &wl_compositor_interface, 4, server,
	compositor_bind);
	wl_global_create(D, &wl_subcompositor_interface, 1, 0,
	subcompositor_bind);
	wl_global_create(D, &wl_data_device_manager_interface, 1, 0,
	data_device_manager_bind);
	wl_global_create(D, &wl_seat_interface, 5, server, seat_bind);
	wl_global_create(D, &wl_output_interface, 3, 0, output_bind);
	wl_display_init_shm(D);
	wl_global_create(D, &xdg_wm_base_interface, 1, server,
	xdg_wm_base_bind);
/*	wl_global_create(D, &zxdg_shell_v6_interface, 1, server,
	xdg_shell_v6_bind);*/
	if (dmabuf)
		wl_global_create(D, &zwp_linux_dmabuf_v1_interface, 3, server,
		 zwp_linux_dmabuf_v1_bind);
	wl_global_create(D, &zwp_fullscreen_shell_v1_interface, 1, NULL,
	zwp_fullscreen_shell_v1_bind);

	wl_display_set_global_filter(D, global_filter, 0);

// Can I move at the beginning of the program (still enter key stuck?)
	server->input = input_setup();
	if (!server->input)
		return EXIT_FAILURE;
	
	legacy_wl_drm_setup(D, screen_get_gpu_fd(server->screen));

	struct wl_event_loop *el = wl_display_get_event_loop(D);
	wl_event_loop_add_fd(el, screen_get_gpu_fd(server->screen),
	WL_EVENT_READABLE, gpu_ev_handler, server);
	wl_event_loop_add_fd(el, input_get_key_fd(server->input),
	WL_EVENT_READABLE, key_ev_handler, server);

	if (argc > 1) {
		int len = argc+4; // amount of spaces + terminating NULL byte + 'exec '
		for (int i=1; i<argc; i++) {
			len += strlen(argv[i]);
		}
		char *client_cmd = malloc(len*sizeof(char*));
		strcpy(client_cmd, "exec ");
		for (int i=1, offset=5; i<argc; i++) {
			strcpy(client_cmd+offset, argv[i]);
			if (i < argc-1) {
				offset = strlen(client_cmd)+1;
				client_cmd[offset-1] = ' ';
			}
		}

		pid_t pid = fork();
		if (!pid) {
			execl("/bin/sh", "/bin/sh", "-c", client_cmd, (char*)NULL);
			/* If execl returns (i.e. /bin/sh doesn't exist) we are
			 * in big trouble here
			 */
		}
	}

//	screen_post(server->screen, 0);

	wl_display_run(D);

/*	vulkan_main(1);
	screen_post(server->screen);
	sleep(1);*/

	wl_display_destroy(D);
	input_release(server->input);
	screen_release(server->screen);
	free(server);
	return 0;
}
