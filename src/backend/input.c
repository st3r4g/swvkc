#define _POSIX_C_SOURCE 200809L
#include <backend/input.h>
#include <backend/dev.h>
#include <util/log.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>

struct input {
	int key_fd;
	int poi_fd;
	int keymap_fd;
	unsigned int keymap_size;

	struct xkb_context *context;
	struct xkb_keymap *keymap;
	struct xkb_state *state;

	struct input_events input_events;
};

void free_keyboard_devices(struct key_dev *key_devs, int count) {
	for (int i=0; i<count; i++) {
		free(key_devs[i].devnode);
	}
	free(key_devs);
}

int create_file(off_t size) {
	static const char template[] = "/swvkc-XXXXXX";
	const char *path;
	char *name;
	int ret;

	path = getenv("XDG_RUNTIME_DIR");
	if (!path) {
		errlog("XDG_RUNTIME_DIR not set");
		errno = ENOENT;
		return -1;
	}

	name = malloc(strlen(path) + sizeof(template));
	if (!name)
		return -1;

	strcpy(name, path);
	strcat(name, template);

	int fd = mkstemp(name);
	if (fd >= 0) {
		long flags = fcntl(fd, F_GETFD);
		fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
		unlink(name);
	}

	free(name);

	if (fd < 0)
		return -1;

	do {
		ret = posix_fallocate(fd, 0, size);
	} while (ret == EINTR);
	if (ret != 0) {
		close(fd);
		errno = ret;
		return -1;
	}
	
	return fd;
}

struct input *input_setup(struct input_events input_events) {
	printf("├─ INPUT (Linux Input Subsystem)\n");
	int count, n;
	struct key_dev *key_devs = find_keyboard_devices(&count);

	if (count > 1) {
		printf("Found multiple keyboards:\n");
		for (int i=0; i<count; i++) {
			printf("(%d) [%s]\n", i, key_devs[i].devnode);
		}
		printf("Choose one: ");
		scanf("%d", &n);
	} else {
		// Handle count == 0
		n = 0;
	}
	boxlog("Keyboard device node: %s", key_devs[n].devnode);

	int count_, n_;
	struct key_dev *key_devs_ = find_pointer_devices(&count_);
	if (count_ > 1) {
		printf("Found multiple pointers:\n");
		for (int i=0; i<count_; i++) {
			printf("(%d) [%s]\n", i, key_devs_[i].devnode);
		}
		printf("Choose one: ");
		scanf("%d", &n_);
	} else {
		// Handle count == 0
		n_ = 0;
	}
	boxlog("Pointer device node: %s", key_devs_[n].devnode);

	struct input *S = calloc(1, sizeof(struct input));
	S->input_events = input_events;
	S->key_fd = open(key_devs[n].devnode, O_RDONLY | O_CLOEXEC);
	if (S->key_fd < 0) {
		fprintf(stderr, "open %s: %s\n", key_devs[n].devnode, strerror(errno));
		return 0;
	}
	S->poi_fd = open(key_devs_[n_].devnode, O_RDONLY | O_CLOEXEC);
	if (S->poi_fd < 0) {
		fprintf(stderr, "open %s: %s\n", key_devs_[n_].devnode, strerror(errno));
		return 0;
	}
	free_keyboard_devices(key_devs, count);
	free_keyboard_devices(key_devs_, count_);

	ioctl(S->key_fd, EVIOCGRAB, 1);

	S->context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!S->context) {
		printf("Cannot create XKB context\n");
	}
	
	struct xkb_rule_names rules = {
		getenv("XKB_DEFAULT_RULES"),
		getenv("XKB_DEFAULT_MODEL"),
		getenv("XKB_DEFAULT_LAYOUT"),
		getenv("XKB_DEFAULT_VARIANT"),
		getenv("XKB_DEFAULT_OPTIONS")
	};

	S->keymap = xkb_map_new_from_names(S->context, &rules,
	XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (S->keymap == NULL) {
		xkb_context_unref(S->context);
		printf("Cannot create XKB keymap\n");
		return 0;
	}

	char *keymap_str = NULL;
	keymap_str = xkb_keymap_get_as_string(S->keymap,
	XKB_KEYMAP_FORMAT_TEXT_V1);
	S->keymap_size = strlen(keymap_str) + 1;
	S->keymap_fd = create_file(S->keymap_size);
	if (S->keymap_fd < 0) {
		printf("creating a keymap file for %u bytes failed\n", S->keymap_size);
		return 0;
	}
	void *ptr = mmap(NULL, S->keymap_size,
		PROT_READ | PROT_WRITE, MAP_SHARED, S->keymap_fd, 0);
	if (ptr == (void*)-1) {
		printf("failed to mmap() %u bytes", S->keymap_size);
		return 0;
	}
	strcpy(ptr, keymap_str);
	free(keymap_str);
	
	S->state = xkb_state_new(S->keymap);

	return S;
}

int input_get_key_fd(struct input *S) {
	return S->key_fd;
}

int input_get_poi_fd(struct input *S) {
	return S->poi_fd;
}

unsigned int input_get_keymap_fd(struct input *S) {
	return S->keymap_fd;
}

unsigned int input_get_keymap_size(struct input *S) {
	return S->keymap_size;
}


int key_ev_handler(int fd, uint32_t mask, void *data) {
	static struct aaa aaa;
	struct input *S = data;

	struct input_event ev;
	read(fd, &ev, sizeof(struct input_event));

	if (ev.type == EV_KEY) {
		if (ev.value == 2)
			return 0;
		aaa.key = ev.code;
		aaa.state = ev.value > 0 ? 1 : 0;
		xkb_keycode_t keycode = aaa.key + 8;
		xkb_state_key_get_utf8(S->state, keycode, aaa.name, 2);
		enum xkb_key_direction direction = aaa.state ? XKB_KEY_DOWN : XKB_KEY_UP;

		xkb_state_update_key(S->state, keycode, direction);

		aaa.mods_depressed = xkb_state_serialize_mods(S->state,
		XKB_STATE_MODS_DEPRESSED);
		aaa.mods_latched = xkb_state_serialize_mods(S->state,
		XKB_STATE_MODS_LATCHED);
		aaa.mods_locked = xkb_state_serialize_mods(S->state,
		XKB_STATE_MODS_LOCKED);
		aaa.group = xkb_state_serialize_layout(S->state,
		XKB_STATE_LAYOUT_EFFECTIVE);
		
		S->input_events.key(&aaa, S->input_events.user_data);
	}

	return 0;
}

bool motion_x = false;
bool motion_y = false;
int32_t abs_x = 0;
int32_t abs_y = 0;

int touchpad_ev_handler(int fd, uint32_t mask, void *data) {
	struct input *S = data;

	struct input_event ev;
	read(fd, &ev, sizeof(struct input_event));

	if (ev.type == EV_KEY && ev.code == BTN_TOUCH && ev.value == 0) {
		motion_x = false, motion_y = false;
		return 0;
	} else if (ev.type == EV_ABS && ev.code == ABS_X) {
		if (motion_x) {
			pointer.x += (ev.value - abs_x)/2;
			S->input_events.motion(0, S->input_events.user_data);
		} else
			motion_x = true;
		abs_x = ev.value;
		return 0;
	} else if (ev.type == EV_ABS && ev.code == ABS_Y) {
		if (motion_y) {
			pointer.y += (ev.value - abs_y)/2;
			S->input_events.motion(0, S->input_events.user_data);
		} else
			motion_y = true;
		abs_y = ev.value;
		return 0;
	} else if (ev.type == EV_SYN) {
		S->input_events.frame(S->input_events.user_data);
		return 0;
	} else if (ev.type == EV_KEY && ev.code == BTN_LEFT) {
		S->input_events.button(0, BTN_LEFT, ev.value,
		 S->input_events.user_data);
		return 0;
	} else
		return 0;
}

void input_release(struct input *S) {
	xkb_state_unref(S->state);
	xkb_keymap_unref(S->keymap);
	xkb_context_unref(S->context);
	close(S->key_fd);
	close(S->poi_fd);
	free(S);
}
