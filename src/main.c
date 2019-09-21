#define _POSIX_C_SOURCE 200809L

#include <swvkc.h>

#include <legacy_wl_drm.h>
#include <backend/input.h>
#include <backend/screen.h>
#include <backend/vulkan.h>
#include <util/log.h>

#include <globals.h> // libswvkc-wl
#include <wayland-server-core.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
	wl_list_init(&server->lss_list);

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
	boxlog("Vulkan dma-buf support: %s", status[dmabuf]);
	boxlog("Vulkan dma-buf with modifiers support: %s", status[dmabuf_mod]);
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
