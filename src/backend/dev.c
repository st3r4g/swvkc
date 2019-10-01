#define _POSIX_C_SOURCE 200809L
#include <backend/dev.h>

#include <libudev.h>

#include <stdlib.h>
#include <string.h>

char *boot_gpu_devpath() {
	char *devpath = 0;
	struct udev *udev = udev_new();
	struct udev_enumerate *enu = udev_enumerate_new(udev);
	udev_enumerate_add_match_sysattr(enu, "boot_vga", "1");
	udev_enumerate_scan_devices(enu);
	struct udev_list_entry *cur;
	udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
		struct udev_device *dev = udev_device_new_from_syspath(udev,
		udev_list_entry_get_name(cur));
		udev_enumerate_unref(enu);
		enu = udev_enumerate_new(udev);
		udev_enumerate_add_match_parent(enu, dev);
		udev_enumerate_add_match_sysname(enu, "card[0-9]");
		udev_enumerate_scan_devices(enu);
		udev_device_unref(dev);
		udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
			dev = udev_device_new_from_syspath(udev,
			udev_list_entry_get_name(cur));
			const char *str = udev_device_get_devnode(dev);
			devpath = malloc((strlen(str)+1)*sizeof(char));
			strcpy(devpath, str);
			udev_device_unref(dev);
			udev_enumerate_unref(enu);
			break;
		}
		break;
	}
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
