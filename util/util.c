#define _POSIX_C_SOURCE 200809L
#include <wayland-server-core.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static enum wl_iterator_result find_keyboard(struct wl_resource *resource,
void *user_data) {
	if (!strcmp(wl_resource_get_class(resource), "wl_keyboard")) {
		struct wl_resource **keyboard_resource = user_data;
		*keyboard_resource = resource;
		return WL_ITERATOR_STOP;
	}
	return WL_ITERATOR_CONTINUE;
}

struct wl_resource *util_wl_client_get_keyboard(struct wl_client *client) {
	struct wl_resource *keyboard_resource = NULL;
	wl_client_for_each_resource(client, find_keyboard, &keyboard_resource);
	return keyboard_resource;
}

char *read_file(const char *path)
{
//	char *buffer = 0;
//	long lenght;
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		fprintf(stderr, "fopen on %s failed\n", path);
		return NULL;
	}

	char *line = NULL;
	size_t len = 0;
	getline(&line, &len, f);

/*	fseek(f, 0, SEEK_END);
	lenght = ftell(f);
	fseek(f, 0, SEEK_SET);
	buffer = malloc(lenght);
	if (buffer == NULL) {
		fprintf(stderr, "malloc failed\n");
		fclose(f);
		return NULL;
	}

	fread(buffer, 1, lenght, f);
	fclose(f);
	buffer[lenght-1] = '\0';*/
	line[len-3] = '\0';
	return line;
}

char *get_a_name(struct wl_client *client) {
	pid_t pid;
	uid_t uid;
	gid_t gid;
	wl_client_get_credentials(client, &pid, &uid, &gid);
	char path[64];
	sprintf(path, "/proc/%d/comm", pid);
	return read_file(path);
}
