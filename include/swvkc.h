#include <stdint.h>

int swvkc_initialize(char *kdevpath, char *pdevpath);
void swvkc_run();
void swvkc_terminate();

struct cursor {
	int32_t x;
	int32_t y;

	int hotspot_x;
	int hotspot_y;
};
