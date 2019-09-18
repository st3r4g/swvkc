struct key_dev {
	char *devnode;
	char *name;
};

char *boot_gpu_devpath();

struct key_dev *find_keyboard_devices(int *count);
