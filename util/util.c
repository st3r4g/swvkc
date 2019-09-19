#define _POSIX_C_SOURCE 200809L
#include <wayland-server-core.h>
#include <util/util.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

static enum wl_iterator_result find_keyboard(struct wl_resource *resource,
void *user_data) {
	if (!strcmp(wl_resource_get_class(resource), "wl_keyboard")) {
		struct wl_resource **keyboard_resource = user_data;
		*keyboard_resource = resource;
		return WL_ITERATOR_STOP;
	}
	return WL_ITERATOR_CONTINUE;
}

static enum wl_iterator_result find_output(struct wl_resource *resource,
void *user_data) {
	if (!strcmp(wl_resource_get_class(resource), "wl_output")) {
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

struct wl_resource *util_wl_client_get_output(struct wl_client *client) {
	struct wl_resource *output_resource = NULL;
	wl_client_for_each_resource(client, find_output, &output_resource);
	return output_resource;
}

/*
 * Assuming that there is a single extension with a given class in the list
 */
struct wl_resource *util_get_extension(struct wl_list *extensions, const char
*class) {
	bool found = false;
	struct extension_node *node;
	wl_list_for_each(node, extensions, link)
		if (!strcmp(wl_resource_get_class(node->resource), class)) {
			found = true;
			break;
		}

	if (found)
		return node->resource;
	else
		return NULL;
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

void dmabuf_save_to_disk(int fd) {
	const int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	unsigned char *buffer = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (buffer == MAP_FAILED) {
		perror("ERROR");
		return;
	}
//	for (int i=5504*768-60; i<5504*768-30; i++)
//		errlog("[%d] byte %i: %x", size, i, buffer[i]);
//	errlog("size %d", );
	FILE* file = fopen("image.data", "w");
	fwrite(buffer, size, 1, file);
	fclose(file);
	munmap(buffer, size);
	/*const int size = 1366*768*4;
	char *buffer = malloc(size);
	read(fd, buffer, 256);
	for (int i=0; i<256; i++)
		errlog("%d", buffer[i]);
	free(buffer);*/
}

static struct timespec t0 = {0};

int a() {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	int s = t.tv_sec - t0.tv_sec;
	int ns = t.tv_nsec - t0.tv_nsec;
	t0 = t;
	return s * 1000 + ns / 1000000;
}
