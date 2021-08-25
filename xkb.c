//#define _POSIX_C_SOURCE 200112L
#define _POSIX_C_SOURCE 200809L

#include <xkbcommon/xkbcommon.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>

static int create_file(off_t size) {
	static const char template[] = "/swvkc-XXXXXX";
	const char *path;
	char *name;
	int ret;

	path = getenv("XDG_RUNTIME_DIR");
	if (!path) {
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

static struct xkb_context *context;
static struct xkb_keymap *keymap;
static int keymap_size;
static int keymap_fd;

void xkb_init() {
	context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	assert(context);
	
	struct xkb_rule_names rules = {
		getenv("XKB_DEFAULT_RULES"),
		getenv("XKB_DEFAULT_MODEL"),
		getenv("XKB_DEFAULT_LAYOUT"),
		getenv("XKB_DEFAULT_VARIANT"),
		getenv("XKB_DEFAULT_OPTIONS")
	};

	keymap = xkb_map_new_from_names(context, &rules,
	XKB_KEYMAP_COMPILE_NO_FLAGS);
	assert(keymap);

	char *keymap_str = NULL;
	keymap_str = xkb_keymap_get_as_string(keymap,
	XKB_KEYMAP_FORMAT_TEXT_V1);
	keymap_size = strlen(keymap_str) + 1;
	keymap_fd = create_file(keymap_size);
	if (keymap_fd < 0) {
		printf("creating a keymap file for %u bytes failed\n", keymap_size);
		return;
	}
	void *ptr = mmap(NULL, keymap_size,
		PROT_READ | PROT_WRITE, MAP_SHARED, keymap_fd, 0);
	if (ptr == (void*)-1) {
		printf("failed to mmap() %u bytes", keymap_size);
		return;
	}
	strcpy(ptr, keymap_str);
	free(keymap_str);

	//state = xkb_state_new(keymap);
}

int32_t xkb_get_keymap_fd() { return keymap_fd; }
uint32_t xkb_get_keymap_size() { return keymap_size; }
