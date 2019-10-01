struct key_dev {
	char *devnode;
	char *name;
};

char *boot_gpu_devpath();

struct key_dev *find_keyboard_devices(int *count);
struct key_dev *find_pointer_devices(int *count);
