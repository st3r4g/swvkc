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

const char *usage = "usage: swvkc [-k <devpath>] [-p <devpath>] [client]\n\
\n\
Experimental Wayland compositor\n\
\n\
positional arguments:\n\
  client                client to exec\n\
\n\
optional arguments:\n\
  -h                    show this help message and exit\n\
  -k <devpath>          keyboard device node\n\
  -p <devpath>          pointer device node\n";

int main(int argc, char *argv[]) {
	char *kdevpath = NULL, *pdevpath = NULL;

	int opt;
	while ((opt = getopt(argc, argv, "hk:p:")) != -1) {
		switch (opt) {
		case 'k': kdevpath = optarg; break;
		case 'p': pdevpath = optarg; break;
		default:
			printf(usage);
			return EXIT_FAILURE;
		}
	}

	if (kdevpath)
	printf("%s\n", kdevpath);
	if (pdevpath)
	printf("%s\n", pdevpath);

	if (swvkc_initialize(kdevpath, pdevpath))
		return EXIT_FAILURE;

	if (argc > optind)
		spawn_client(argv+optind);

	swvkc_run();

	swvkc_terminate();
}
