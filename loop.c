#define _POSIX_C_SOURCE 2

#include "input.h"
#include "atomic.h"
#include "modeset.h"
#include "wayland.h"
#include "xkb.h"

#include <skalibs/iopause.h>
#include <skalibs/strerr.h>

#include <linux/input-event-codes.h>

#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FDS 3

static int kill1;

static void spawn_client(char **argv) {
	if (fork() == 0) {
		execvp(argv[0], argv);
		fprintf(stderr, "execvp %s: %s\n", argv[0], strerror(errno));
		_exit(EXIT_FAILURE);
		/* NOTREACHED */
	}
}

static int handler_key(struct libinput_event *event) {
	struct libinput_event_keyboard *event_keyboard = 
	libinput_event_get_keyboard_event(event);
	assert(event_keyboard);

	uint32_t key = libinput_event_keyboard_get_key(event_keyboard);
	int state = libinput_event_keyboard_get_key_state(event_keyboard); // WARNING

	xkb_update(key, state);

	unsigned int mods_depressed, mods_latched, mods_locked, group;
	xkb_get_modifiers(&mods_depressed, &mods_latched, &mods_locked, &group);

	if (xkb_test_ctrlalt() && key == (kill1 ? KEY_DELETE : KEY_BACKSPACE))
		return 1; // exit
	if (xkb_test_ctrlalt() && key == KEY_ENTER) {
		const char* child_argv[] = {"weston-terminal", NULL};
		spawn_client((char**)child_argv); // emergency terminal
		return 0;
	}

	wayland_send_key_and_mods(key, state, mods_depressed, mods_latched, mods_locked, group);
	return 0;
}

static void handler_pointer(struct libinput_event *event) {

}

static int handler_input(struct libinput *li) {
	int r;
	r = libinput_dispatch(li);
	assert(!r);

	int exit = 0;

	struct libinput_event *event;
	while ((event = libinput_get_event(li))) {
		switch (libinput_event_get_type(event)) {
		case LIBINPUT_EVENT_KEYBOARD_KEY:
			exit = exit || handler_key(event);
		break;
		case LIBINPUT_EVENT_POINTER_MOTION:
			handler_pointer(event);
		break;
		default: break;
		}
		libinput_event_destroy(event);
	}

	return exit;
}

static void check_3_open() {
	if (fcntl(3, F_GETFD) < 0) strerr_die1x(1, "readiness notification");
}

const char *usage = "usage: xyzzy [-k] [client]\n\
\n\
Experimental Wayland system compositor\n\
\n\
positional arguments:\n\
  client                client to exec\n\
\n\
optional arguments:\n\
  -h                    show this help message and exit\n\
  -k                    kill PID 1 on exit (becomes Ctrl+Alt+Del)\n\
  -3                    notify readiness on fd 3\n";
int main(int argc, char *argv[]) {
	int notif = 0;

	int opt;
	while ((opt = getopt(argc, argv, "hk3")) != -1) {
		switch (opt) {
		case 'k': kill1 = 1; break;
		case '3': check_3_open(); notif = 1; break;
		default:
			fprintf(stderr, usage);
			return EXIT_FAILURE;
		}
	}

	struct libinput *li = input_init();
	modeset_init();
	atomic_init();
	wayland_init();
	xkb_init();

	if (notif) {
		write(3, "\n", 1); // ready to accept connections
		close(3);
	}

	if (argc > optind) spawn_client(argv+optind);

	iopause_fd x[FDS] = {
		{ libinput_get_fd(li), IOPAUSE_READ, 0 },
		{ wayland_get_fd(), IOPAUSE_READ, 0 },
		{ modeset_get_fd(), IOPAUSE_READ, 0 },
	};

	int exit = 0;
	while (!exit) {
		wayland_flush();
		iopause(x, FDS, NULL, NULL);
		if (x[0].revents & IOPAUSE_READ) exit = handler_input(li);
		if (x[1].revents & IOPAUSE_READ) wayland_read();
		if (x[2].revents & IOPAUSE_READ) drm_read();
	}

	modeset_cleanup();
	input_fini(li);

	if (kill1) kill(1, SIGINT);
}
