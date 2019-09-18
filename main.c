#define _POSIX_C_SOURCE 200809L

#include <swvkc.h>
#include <globals.h>

#include <legacy_wl_drm.h> // legacy
#include <backend/input.h>
#include <backend/screen.h>
#include <backend/vulkan.h>
#include <core/keyboard.h>
#include <core/wl_surface.h>
#include <core/subsurface.h>
#include <extensions/linux-dmabuf-unstable-v1/zwp_linux_dmabuf_v1.h>
#include <extensions/xdg_shell/xdg_surface.h>
#include <extensions/xdg_shell/xdg_toplevel.h>
#include <util/log.h>
#include <util/util.h>

#include <xdg-shell-server-protocol.h>

#include <linux/input-event-codes.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

struct surface_node {
	struct surface *surface;
	struct surface *child; // XXX: temporary
	struct wl_resource *keyboard;
	struct wl_list link;
};

struct bufres_node {
	struct wl_resource *bufres;
	struct wl_list link;
};

struct surface *focused_surface(struct server *server) {
	struct surface_node *node;
	node = wl_container_of(server->mapped_surfaces_list.next, node, link);
	return node->child ? node->child : node->surface; // XXX: temporary
}

struct wl_resource *focused_surface_keyboard(struct server *server) {
	struct surface_node *node;
	node = wl_container_of(server->mapped_surfaces_list.next, node, link);
	return node->keyboard;
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

	struct box screen_size = screen_get_dimensions(screen);
	if ((int32_t)width == screen_size.width && (int32_t)height == screen_size.height)
		client_buffer_on_primary(screen, fb);
	else if (screen_is_overlay_supported(screen))
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

	if (surface->role == ROLE_SUBSURFACE) {
		struct subsurface *subsurface = surface->role_object;
		struct surface *parent = wl_resource_get_user_data(subsurface->current->parent);
		struct surface_node *node;
		wl_list_for_each(node, &server->mapped_surfaces_list, link)
			if (node->surface == parent) {
				node->child = surface;
				break;
			}
		return;
	}

	if (!wl_list_empty(&server->mapped_surfaces_list)) {
		struct surface *old = focused_surface(server);
		struct wl_resource *keyboard = focused_surface_keyboard(server);
		if (keyboard)
			wl_keyboard_send_leave(keyboard, 0, old->resource);
	}
	errlog("A surface has been mapped");

	struct wl_array array;
	wl_array_init(&array); //Need the currently pressed keys
	struct wl_client *client = wl_resource_get_client(surface->resource);
	struct wl_resource *keyboard = NULL;
	keyboard = util_wl_client_get_keyboard(client);
	if (keyboard)
		wl_keyboard_send_enter(keyboard, 0, surface->resource, &array);

	struct surface_node *node = malloc(sizeof(struct surface_node));
	node->surface = surface;
	node->child = NULL;
	node->keyboard = keyboard;
	wl_list_insert(&server->mapped_surfaces_list, &node->link);
}

void surface_unmap_notify(struct surface *surface, void *user_data) {
	struct server *server = user_data;

	if (surface->role == ROLE_SUBSURFACE) {
		struct subsurface *subsurface = surface->role_object;
		struct surface *parent = wl_resource_get_user_data(subsurface->current->parent);
		struct surface_node *node;
		wl_list_for_each(node, &server->mapped_surfaces_list, link)
			if (node->surface == parent) {
				node->child = NULL;
				break;
			}
		return;
	}

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
		struct wl_resource *keyboard = focused_surface_keyboard(server);
		if (keyboard)
			wl_keyboard_send_enter(keyboard, 0, new->resource, &array);
	}
}

void surface_contents_update_notify(struct surface *surface, void *user_data) {
	struct server *server = user_data;
	struct wl_resource *buffer = surface->current->buffer;
	/*
	 * If the buffer has been detached, do nothing
	 */
	if (!buffer)
		return;

	if (screen_page_flip_is_pending(server->screen)) {
		errlog("WARNING: contents update discarded (page flip is pending)");
		wl_buffer_send_release(buffer);
	} else if (surface != focused_surface(server)) {
		errlog("WARNING: contents update discarded (surface is not focused)");
		wl_buffer_send_release(buffer);
	} else {
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

		screen_atomic_commit(screen);
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

// XXX: ugly
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

void server_change_focus(struct server *self, struct surface_node *node) {
	struct surface *focused = focused_surface(self);
	struct wl_resource *keyboard = focused_surface_keyboard(self);
	if (keyboard)
		wl_keyboard_send_leave(keyboard, 0, focused->resource);
	wl_list_remove(&node->link);
	wl_list_insert(&self->mapped_surfaces_list, &node->link);
	struct wl_array array;
	wl_array_init(&array); //Need the currently pressed keys
	if (node->keyboard)
		wl_keyboard_send_enter(node->keyboard, 0, node->surface->resource, &array);
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
/*
 * If a dmabuf has been scanned out directly, the VBI (now) is the right time to
 * release it.
 * TODO: Find a better solution to avoid releasing a destroyed buffer by design
 *       without having to check each dmabuf buffer destruction.
 */
	if (wl_list_length(&server->bufres_list) > 1) {
		struct bufres_node *node;
		node = wl_container_of(server->bufres_list.prev, node, link);
		wl_buffer_send_release(node->bufres);
		wl_list_remove(&node->link);
		free(node);
	}
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
			struct wl_resource *keyboard = focused_surface_keyboard(server);
			if (keyboard) {
				struct keyboard *data = wl_resource_get_user_data(keyboard);
				if (data)
				keyboard_send(data, &aaa);
			}
		}
	}
	return 0;
}

void spawn_client(char **argv) {
	if (fork() == 0) {
		execvp(argv[0], argv);
		fprintf(stderr, "execvp %s: %s\n", argv[0], strerror(errno));
		errlog("Could not start client %s", argv[0]);
		_exit(EXIT_FAILURE);
		/* NOTREACHED */
	}
}

int main(int argc, char *argv[]) {
	struct server *server = calloc(1, sizeof(struct server));
	wl_list_init(&server->mapped_surfaces_list);
	wl_list_init(&server->bufres_list);

	bool dmabuf = false, dmabuf_mod = false;
	if (vulkan_init(&dmabuf, &dmabuf_mod) < 0) {
		errlog("Could not setup the renderer (Vulkan)");
		return EXIT_FAILURE;
	}

	server->screen = screen_setup(vblank_notify, server, dmabuf_mod);
	if (!server->screen) {
		errlog("Could not setup screen");
		return EXIT_FAILURE;
	}
/*
 * Fast computers could be affected by the 'stuck enter key' bug
 * (due to EVIOCGRAB inside input_setup)
 */
	server->input = input_setup();
	if (!server->input) {
		errlog("Could not setup input");
		return EXIT_FAILURE;
	}

	printf("├─ SWVKC (Wayland compositor)\n");
	const char *status[] = {"DISABLED", "enabled"};
	boxlog("Buffer manager: %s", screen_get_bufmgr_impl(server->screen));
	boxlog("dma-buf support: %s", status[dmabuf]);
	boxlog("dma-buf with modifiers support: %s", status[dmabuf_mod]);
	printf("└\n");

	vulkan_create_screen_image(screen_get_back_buffer(server->screen),
	                           screen_get_front_buffer(server->screen));

	server->display = wl_display_create();
	struct wl_display *D = server->display;

	const char *socket = wl_display_add_socket_auto(D);
	if (socket == NULL) {
		errlog("Could not create socket");
		return EXIT_FAILURE;
	}

	setenv("WAYLAND_DISPLAY", socket, 0);
	setenv("QT_QPA_PLATFORM", "wayland-egl", 0);

	create_globals(server, D, dmabuf);
/*
 * Creates the `wl_drm` global required for authenticating clients to DRM
 */
	legacy_wl_drm_setup(D, screen_get_gpu_fd(server->screen));

	struct wl_event_loop *el = wl_display_get_event_loop(D);
	wl_event_loop_add_fd(el, screen_get_gpu_fd(server->screen),
	WL_EVENT_READABLE, gpu_ev_handler, server);
	wl_event_loop_add_fd(el, input_get_key_fd(server->input),
	WL_EVENT_READABLE, key_ev_handler, server);

	if (argc > 1)
		spawn_client(argv+1);

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
