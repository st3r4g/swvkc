#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void errlog(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	struct timespec tp;
	unsigned int time;
	clock_gettime(CLOCK_REALTIME, &tp);
	time = (tp.tv_sec * 1000000L) + (tp.tv_nsec / 1000);

	fprintf(stderr, "[%10.3f] ", time / 1000.0);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}
