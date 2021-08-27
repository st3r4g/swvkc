#define _POSIX_C_SOURCE 2

#include "input.h"
#include "atomic.h"
#include "modeset.h"
#include "wayland.h"
#include "xkb.h"

#include <skalibs/iopause.h>

#include <linux/input-event-codes.h>

#include <assert.h>
#include <signal.h>

#include <errno.h>
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

static int handler_input() {
	uint32_t key;
	int state;
	if (input_read(&key, &state)) {
		xkb_update(key, state);

		unsigned int mods_depressed, mods_latched, mods_locked, group;
		xkb_get_modifiers(&mods_depressed, &mods_latched, &mods_locked, &group);

		if (xkb_test_ctrlalt() && key == (kill1 ? KEY_DELETE : KEY_BACKSPACE))
			return 1; // exit

		wayland_send_key_and_mods(key, state, mods_depressed, mods_latched, mods_locked, group);
	}

	return 0;
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
  -k                    kill PID 1 on exit (becomes Ctrl+Alt+Del)\n";
int main(int argc, char *argv[]) {
	int opt;
	while ((opt = getopt(argc, argv, "hk")) != -1) {
		switch (opt) {
		case 'k': kill1 = 1; break;
		default:
			fprintf(stderr, usage);
			return EXIT_FAILURE;
		}
	}

	input_init();
	modeset_init();
	atomic_init();
	wayland_init();
	xkb_init();

	if (argc > optind) spawn_client(argv+optind);

	iopause_fd x[FDS] = {
		{ input_get_fd(), IOPAUSE_READ, 0 },
		{ wayland_get_fd(), IOPAUSE_READ, 0 },
		{ modeset_get_fd(), IOPAUSE_READ, 0 },
	};

	int exit = 0;
	while (!exit) {
		wayland_flush();
		iopause(x, FDS, NULL, NULL);
		if (x[0].revents & IOPAUSE_READ) exit = handler_input();
		if (x[1].revents & IOPAUSE_READ) wayland_read();
		if (x[2].revents & IOPAUSE_READ) drm_read();
	}

	modeset_cleanup();
	input_fini();

	if (kill1) kill(1, SIGINT);
}
