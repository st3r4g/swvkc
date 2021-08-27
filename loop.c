#define _POSIX_C_SOURCE 1

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
		if (key == KEY_Q && state == 1)
			return 1; // exit
		wayland_send_key(key, state);
	}

	return 0;
}

int main(int argc, char *argv[]) {
	input_init();
	modeset_init();
	atomic_init();
	wayland_init();
	xkb_init();

	if (argc > 0) spawn_client(argv+1);

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
	//kill(1, SIGINT);
}
