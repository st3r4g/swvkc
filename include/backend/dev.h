#include <stdint.h>
struct key_dev {
	char *devnode;
	char *name;
};

char *gpu_devpath(int64_t major, int64_t minor);

struct key_dev *find_keyboard_devices(int *count);
struct key_dev *find_pointer_devices(int *count);
