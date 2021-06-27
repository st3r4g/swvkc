#define _POSIX_C_SOURCE 200809L
#include <backend/dev.h>

#include <libudev.h>

#include <stdlib.h>
#include <string.h>

char *gpu_devpath(int64_t major, int64_t minor) {
	char *devpath = 0;
	struct udev *udev = udev_new();
	dev_t dev = makedev(major, minor);
	struct udev_device *udev_dev = udev_device_new_from_devnum(udev, 'c', dev);
	devpath = strdup(udev_device_get_devnode(udev_dev));
	udev_device_unref(udev_dev);
	udev_unref(udev);
	return devpath;
}

struct key_dev *find_keyboard_devices(int *count) {
	struct udev *udev = udev_new();
	struct udev_enumerate *enu = udev_enumerate_new(udev);
	udev_enumerate_add_match_property(enu, "ID_INPUT_KEYBOARD", "1");
	udev_enumerate_add_match_sysname(enu, "event*");
	udev_enumerate_scan_devices(enu);
	struct udev_list_entry *cur;
	*count = 0;
	udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
		(*count)++;
	}
	struct key_dev *key_devs = calloc(*count, sizeof(struct key_dev));
	int i=0;
	udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
		struct udev_device *dev = udev_device_new_from_syspath(udev,
		udev_list_entry_get_name(cur));
		const char *devnode = udev_device_get_devnode(dev);
		key_devs[i].devnode = malloc((strlen(devnode)+1)*sizeof(char));
		strcpy(key_devs[i].devnode, devnode);
		i++;
	}
	return key_devs;
}

struct key_dev *find_pointer_devices(int *count) {
	struct udev *udev = udev_new();
	struct udev_enumerate *enu = udev_enumerate_new(udev);
	udev_enumerate_add_match_property(enu, "ID_INPUT_MOUSE", "1");
	udev_enumerate_add_match_property(enu, "ID_INPUT_TOUCHPAD", "1");
	udev_enumerate_add_match_sysname(enu, "event*");
	udev_enumerate_scan_devices(enu);
	struct udev_list_entry *cur;
	*count = 0;
	udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
		(*count)++;
	}
	struct key_dev *key_devs = calloc(*count, sizeof(struct key_dev));
	int i=0;
	udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
		struct udev_device *dev = udev_device_new_from_syspath(udev,
		udev_list_entry_get_name(cur));
		const char *devnode = udev_device_get_devnode(dev);
		key_devs[i].devnode = malloc((strlen(devnode)+1)*sizeof(char));
		strcpy(key_devs[i].devnode, devnode);
		i++;
	}
	return key_devs;
}
