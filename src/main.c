#define _POSIX_C_SOURCE 200809L

#include <swvkc.h>

#include <util/log.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void spawn_client(char **argv) {
	if (fork() == 0) {
		execvp(argv[0], argv);
		fprintf(stderr, "execvp %s: %s\n", argv[0], strerror(errno));
		errlog("Could not start client %s", argv[0]);
		_exit(EXIT_FAILURE);
		/* NOTREACHED */
	}
}

int main(int argc, char *argv[]) {
	if (swvkc_initialize())
		return EXIT_FAILURE;

	if (argc > 1)
		spawn_client(argv+1);

	swvkc_run();

	swvkc_terminate();
}
