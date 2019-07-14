#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <protocols/xdg-shell-server-protocol.h>
#include <protocols/xdg-shell-unstable-v6-server-protocol.h>
#include <protocols/linux-dmabuf-unstable-v1-server-protocol.h>
#include <protocols/fullscreen-shell-unstable-v1.h>

#include <legacy_wl_drm.h> // legacy
#include <backend/input.h>
#include <backend/screen.h>
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
#include <extensions/linux-dmabuf-unstable-v1/zwp_linux_dmabuf_v1.h>
#include <util/box.h>

#include <backend/vulkan.h>

struct server {
	struct wl_display *display;

	struct input *input;
	struct screen *screen;

	struct wl_list xdg_wm_base_list;
//	struct wl_list xdg_shell_v6_list;
	struct wl_list seat_list;

	struct wl_list xdg_surface_list;
};

struct server *server;

static void resource_created_notify(struct wl_listener *listener, void *data) {
	struct wl_resource *resource = data;
	const char *class = wl_resource_get_class(resource);
	if (!strcmp(class, "xdg_surface"))
		errlog("Created resource %s", class);
}

struct wl_listener resource_created = {.notify = resource_created_notify};

static void client_created_notify(struct wl_listener *listener, void *data) {
	struct wl_client *client = data;
	wl_client_add_resource_created_listener(client, &resource_created);
}

struct wl_listener client_created = {.notify = client_created_notify};

void server_window_create(struct server *server, struct xdg_surface0
*xdg_surface) {
	struct wl_list * head = &server->xdg_surface_list;
	if (!wl_list_empty(head)) {
	struct xdg_surface0 *an_xdg_surface;
	an_xdg_surface = wl_container_of(head->next, an_xdg_surface, link);
	if (an_xdg_surface->self)
		wl_keyboard_send_leave(an_xdg_surface->keyboard, 0,
		an_xdg_surface->surface);
	}
	wl_list_insert(head, &xdg_surface->link);

}

void server_window_destroy(struct server *server, struct xdg_surface0
*xdg_surface) {
	struct wl_array array;
	wl_array_init(&array); //Need the currently pressed keys
//	wl_keyboard_send_leave(resource, 0, xdg_surface->surface);
	wl_list_remove(&xdg_surface->link);
	struct wl_list * head = &server->xdg_surface_list;
	if (!wl_list_empty(head)) {
	struct xdg_surface0 *an_xdg_surface;
	an_xdg_surface = wl_container_of(head->next, an_xdg_surface, link);
	if (an_xdg_surface->self)
		wl_keyboard_send_enter(an_xdg_surface->keyboard, 0,
		an_xdg_surface->surface, &array);
	}
}

void server_change_focus(struct server *server) {
	struct wl_list * head = &server->xdg_surface_list;
	struct xdg_surface0 *an_xdg_surface;
	an_xdg_surface = wl_container_of(head->next, an_xdg_surface, link);
	if (an_xdg_surface->self) {
		wl_keyboard_send_leave(an_xdg_surface->keyboard, 0,
		an_xdg_surface->surface);
	}
	wl_list_remove(&an_xdg_surface->link);
	wl_list_insert(head->prev, &an_xdg_surface->link);
	struct wl_array array;
	wl_array_init(&array); //Need the currently pressed keys
	struct xdg_surface0 *an_xdg_surface2;
	an_xdg_surface2 = wl_container_of(head->next, an_xdg_surface2, link);
	if (an_xdg_surface2->self) {
		wl_keyboard_send_enter(an_xdg_surface2->keyboard, 0,
		an_xdg_surface2->surface, &array);
	}
}

struct screen *server_get_screen(struct server *server) {
	return server->screen;
}

/*void server_focus(struct xdg_surface *new) {
	struct server *S = new->server;
	if (S->focused)
		wl_keyboard_send_leave(S->focused->keyboard, 0, S->focused->surface);
	struct wl_array array;
	wl_array_init(&array);
	wl_keyboard_send_enter(new->keyboard, 0, new->surface, &array);
	S->focused = new;
}*/

static void vblank_notify(int gpu_fd, unsigned int sequence, unsigned int
tv_sec, unsigned int tv_usec, void *user_data) {
//	printf("VBLANK HANDLER %p\n", user_data);
/*	struct xdg_surface0 *xdg_surface;
	wl_list_for_each(xdg_surface, &server->xdg_surface_list, link) {
		struct surface *surface =
		wl_resource_get_user_data(xdg_surface->surface);
		if (surface->frame) {
			unsigned int ms = tv_sec * 1000 + tv_usec / 1000;
			wl_callback_send_done(surface->frame, (uint32_t)ms);
			wl_resource_destroy(surface->frame);
			surface->frame = 0;
		}
	}*/
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);

	struct xdg_surface0 *xdg_surface;
	wl_list_for_each(xdg_surface, &server->xdg_surface_list, link) {
		struct surface *surface =
		wl_resource_get_user_data(xdg_surface->surface);
		if (surface->frame) {
			unsigned int ms = time.tv_sec * 1000 + time.tv_nsec / 1000000;
			wl_callback_send_done(surface->frame, (uint32_t)ms);
			wl_resource_destroy(surface->frame);
			surface->frame = 0;
		}
	}
}

static int gpu_ev_handler(int fd, uint32_t mask, void *data) {
	drm_handle_event(fd);

	return 0;
}

static int key_ev_handler(int key_fd, uint32_t mask, void *data) {
	struct server *server = data;
	struct aaa aaa;
	if (input_handle_event(server->input, &aaa)) {
		if (aaa.key == 59) { //F1
			wl_display_terminate(server->display);
			return 0;
		}
		if (aaa.key == 60 && aaa.state == 1) { //F2
			server_change_focus(server);
			return 0;
		}
		if (!wl_list_empty(&server->xdg_surface_list)) {
			struct xdg_surface0 *xdg_surface;
			xdg_surface = wl_container_of(server->xdg_surface_list.next, xdg_surface, link);
			if (xdg_surface->keyboard) //TODO: bad
			if (wl_resource_get_user_data(xdg_surface->keyboard))
				keyboard_send(wl_resource_get_user_data(xdg_surface->keyboard), &aaa);
		}
	}
	return 0;
}

static void compositor_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&wl_compositor_interface, version, id);
	compositor_new(resource, server);
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
	struct xdg_wm_base *xdg_wm_base = xdg_wm_base_new(resource, server);
	wl_list_insert(&server->xdg_wm_base_list, &xdg_wm_base->link);
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
	bool *dmabuf_mod = data;
	struct wl_resource *resource = wl_resource_create(client,
	&zwp_linux_dmabuf_v1_interface, version, id);
	zwp_linux_dmabuf_v1_new(resource, *dmabuf_mod);
}

static void zwp_fullscreen_shell_v1_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
}

int main(int argc, char *argv[]) {
	//struct server *server = calloc(1, sizeof(struct server));
	server = calloc(1, sizeof(struct server));
	wl_list_init(&server->seat_list);
	wl_list_init(&server->xdg_wm_base_list);
	wl_list_init(&server->xdg_surface_list);
//	wl_list_init(&server->xdg_shell_v6_list);

	bool dmabuf = false, dmabuf_mod = false;
	vulkan_init(&dmabuf, &dmabuf_mod);
	const char *words[] = {"DISABLED", "enabled"};
	errlog("swvkc DMABUF support: %s", words[dmabuf]);
	errlog("swvkc DMABUF with MODIFIERS support: %s", words[dmabuf_mod]);

	server->screen = screen_setup(vblank_notify, server, dmabuf_mod);
	if (!server->screen)
		return EXIT_FAILURE;

	// maybe move to screen
	struct box box= screen_get_dimensions(server->screen);
	vulkan_main(0,screen_get_bo_fd(server->screen), box.width, box.height,
	screen_get_bo_stride(server->screen),
	screen_get_bo_modifier(server->screen));

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
		wl_global_create(D, &zwp_linux_dmabuf_v1_interface, 3,
		&dmabuf_mod, zwp_linux_dmabuf_v1_bind);
/*	wl_global_create(D, &zwp_fullscreen_shell_v1_interface, 1, NULL,
	zwp_fullscreen_shell_v1_bind);*/

	wl_display_add_client_created_listener(D, &client_created);

// Can I move at the beginning of the program (still enter key stucks?)
	server->input = input_setup();
	if (!server->input)
		return EXIT_FAILURE;
	
	legacy_wl_drm_setup(D, screen_get_gbm_device(server->screen));

	struct wl_event_loop *el = wl_display_get_event_loop(D);
	wl_event_loop_add_fd(el, screen_get_gpu_fd(server->screen),
	WL_EVENT_READABLE, gpu_ev_handler, server);
	wl_event_loop_add_fd(el, input_get_key_fd(server->input),
	WL_EVENT_READABLE, key_ev_handler, server);

	if (argc > 1) {
		pid_t pid = fork();
		if (!pid) {
			execl("/bin/sh", "/bin/sh", "-c", argv[1], (char*)0);
			/* If execl returns (i.e. /bin/sh doesn't exist) we are
			 * in big trouble here
			 */
		}
	}

	screen_post(server->screen);

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
