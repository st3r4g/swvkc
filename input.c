#include <libinput.h>
#include <libudev.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

static int open_restricted(const char *path, int flags, void *user_data) {
	int fd = open(path, flags);
	return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data) {
	close(fd);
}

static struct libinput * libinput;

void input_init() {
	int r;

	const struct libinput_interface interface = {
		.open_restricted = open_restricted,
		.close_restricted = close_restricted,
	};
	struct udev *udev = udev_new();
	assert(udev);
	libinput = libinput_udev_create_context(&interface, NULL, udev);
	assert(libinput);
	r = libinput_udev_assign_seat(libinput, "seat0");
	assert(!r);

	/*const char *path = "/dev/input/event0";
	libinput = libinput_path_create_context(&interface, NULL);
	assert(libinput);
	struct libinput_device *device = libinput_path_add_device(libinput, path);
	assert(device);*/
}

int input_get_fd() {
	return libinput_get_fd(libinput);
}

int input_read(uint32_t *key, int *state) {
	int r, select = 0;

	r = libinput_dispatch(libinput);
	assert(!r);

	struct libinput_event *event;
	while ((event = libinput_get_event(libinput))) {
		switch (libinput_event_get_type(event)) {
			case LIBINPUT_EVENT_KEYBOARD_KEY:;
				struct libinput_event_keyboard *event_keyboard = 
				libinput_event_get_keyboard_event(event);
				assert(event_keyboard);
				*key = libinput_event_keyboard_get_key(event_keyboard);
				*state = libinput_event_keyboard_get_key_state(event_keyboard); // WARNING
				select = 1;
			break;
			default:;
			break;
		}
		libinput_event_destroy(event);
	}
	return select;
}

void input_fini() {

}
