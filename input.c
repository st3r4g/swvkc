#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <libudev.h>

#include "input.h"

static int open_restricted(const char *path, int flags, void *user_data) {
	int fd = open(path, flags);
	return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data) {
	close(fd);
}

struct libinput *input_init() {
	int r;

	const struct libinput_interface interface = {
		.open_restricted = open_restricted,
		.close_restricted = close_restricted,
	};

	struct udev *udev = udev_new();
	assert(udev);
	struct libinput *li = libinput_udev_create_context(&interface, NULL, udev);
	assert(li);
	r = libinput_udev_assign_seat(li, "seat0");
	assert(!r);

	return li;

	/*const char *path = "/dev/input/event0";
	libinput = libinput_path_create_context(&interface, NULL);
	assert(libinput);
	struct libinput_device *device = libinput_path_add_device(libinput, path);
	assert(device);*/
}

void input_fini(struct libinput *li) {
	//li = libinput_unref(li);
	//assert(!li);
}
