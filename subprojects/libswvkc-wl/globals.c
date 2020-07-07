#define _POSIX_C_SOURCE 200809L

#include <core/compositor.h>
#include <core/data_device_manager.h>
#include <core/output.h>
#include <core/seat.h>
#include <core/wl_subcompositor.h>
#include <extensions/xdg_shell/xdg_wm_base.h>
#include <extensions/linux-dmabuf-unstable-v1/zwp_linux_dmabuf_v1.h>
#include <extensions/linux-explicit-synchronization-v1/zwp_linux_explicit_synchronization_v1.h>
#include <extensions/fullscreen-shell-unstable-v1/zwp_fullscreen_shell_v1.h>
#include <extensions/server-decoration/org_kde_kwin_server_decoration_manager.h>

#include <util/util.h>

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <xdg-shell-server-protocol.h>
#include <linux-dmabuf-unstable-v1-server-protocol.h>
#include <linux-explicit-synchronization-unstable-v1-server-protocol.h>
#include <fullscreen-shell-unstable-v1-server-protocol.h>
#include <server-decoration-server-protocol.h>

#include <stdlib.h>
#include <string.h>

#include <stdio.h>
/*
 * The Wayland objects tree
 */
struct node {
	struct wl_resource *object;
	struct node *child;
	struct node *next;
} root;

struct node *node_add(struct node *parent, struct wl_resource *object) {
	struct node *new = malloc(sizeof(struct node));
	new->object = object;
	new->child = NULL;
	new->next = NULL;

	struct node *el = parent;
	if (!el->child) {
		el->child = new;
		return new;
	}
	for (el = el->child; el->next; )
		el = el->next;
	el->next = new;
	return new;
}

void node_print(struct node *node) {
	if (!node->object)
		return;
	struct wl_client *cl = wl_resource_get_client(node->object);
	pid_t pid;
	uid_t uid;
	gid_t gid;
	wl_client_get_credentials(cl, &pid, &uid, &gid);
	printf("%s@%u (%d)\n", wl_resource_get_class(node->object), wl_resource_get_id(node->object), pid);

}

void tree_print(struct node *root) {
	struct node *cur = root;
	while (cur) {
		node_print(cur);
		cur = cur->child;
/*		while (cur) {
			cur = cur->next;
		}*/
	}
}

struct wl_list data_device_manager_list;

struct fake {
	struct wl_resource *resource;
	struct wl_list link;
};

void print_resource(struct wl_resource *resource, int lv, FILE* s) {
	const char *tabs[] = {"", "  ", "    "};
	struct wl_client *cl = wl_resource_get_client(resource);
	pid_t pid;
	uid_t uid;
	gid_t gid;
	wl_client_get_credentials(cl, &pid, &uid, &gid);
	printf("%s%s@%u (%d)\n", tabs[lv], wl_resource_get_class(resource), wl_resource_get_id(resource), pid);
	fprintf(s, "%s%s@%u (%d)\n", tabs[lv], wl_resource_get_class(resource), wl_resource_get_id(resource), pid);
}

#include <core/data_device.h>

void xxx() {
	FILE *pappo = fopen("/tmp/swvkclog.txt", "w");
	struct data_device_manager *x;
	wl_list_for_each(x, &data_device_manager_list, link) {
		print_resource(x->resource, 0, pappo);
		struct data_device *y;
		wl_list_for_each(y, &x->data_device_list, link) {
			print_resource(y->resource, 1, pappo);
			struct fake *w;
			wl_list_for_each(w, &y->data_offer_list, link) {
				print_resource(w->resource, 2, pappo);
			}
		}
		struct fake *z;
		wl_list_for_each(z, &x->data_source_list, link) {
			print_resource(z->resource, 1, pappo);
		}
	}
	fflush(pappo);
	fclose(pappo);
}

/*
 * Defined in main.c
 */
extern void surface_map_notify(struct surface *surface, void *user_data);
extern void surface_unmap_notify(struct surface *surface, void *user_data);
extern void surface_contents_update_notify(struct surface *surface, void *user_data);
extern void xdg_toplevel_init_notify(struct xdg_toplevel_data *xdg_toplevel, void
*user_data);
extern void buffer_dmabuf_create_notify(struct wl_buffer_dmabuf_data *dmabuf, void
*user_data);
extern void buffer_dmabuf_destroy_notify(struct wl_buffer_dmabuf_data *dmabuf, void
*user_data);
extern void keyboard_init_notify(struct keyboard *keyboard, void *user_data);

static void compositor_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_compositor_interface, version, id);
	struct surface_events surface_events = {
		.map = surface_map_notify,
		.unmap = surface_unmap_notify,
		.contents_update = surface_contents_update_notify,
		.user_data = data
	};
	compositor_new(resource, surface_events);
}

static void data_device_manager_bind(struct wl_client *client, void *data,
uint32_t version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_data_device_manager_interface, version, id);
	struct data_device_manager *new = data_device_manager_new(resource);
	wl_list_insert(&data_device_manager_list, &new->link);
}

static void seat_bind(struct wl_client *client, void *data, uint32_t version,
uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_seat_interface, version, id);
	struct keyboard_events keyboard_events = {
		.init = keyboard_init_notify,
		.user_data = data
	};
	seat_new(resource, keyboard_events);
}

static void subcompositor_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_subcompositor_interface, version, id);
	wl_subcompositor_new(resource);
}

static void output_bind(struct wl_client *client, void *data, uint32_t version,
uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_output_interface, version, id);
	output_new(resource);
}

static void xdg_wm_base_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&xdg_wm_base_interface, version, id);
	struct xdg_toplevel_events xdg_toplevel_events = {
		.init = xdg_toplevel_init_notify,
		.user_data = data
	};
	xdg_wm_base_new(resource, xdg_toplevel_events);
}

static void zwp_linux_dmabuf_v1_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&zwp_linux_dmabuf_v1_interface, version, id);
	struct buffer_dmabuf_events buffer_dmabuf_events = {
		.create = buffer_dmabuf_create_notify,
		.destroy = buffer_dmabuf_destroy_notify,
		.user_data = data
	};
	zwp_linux_dmabuf_v1_new(resource, buffer_dmabuf_events);
}

static void zwp_linux_explicit_synchronization_v1_bind(struct wl_client *client,
void *data, uint32_t version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&zwp_linux_explicit_synchronization_v1_interface, version, id);
	linux_explicit_synchronization_new(resource);
}

static void zwp_fullscreen_shell_v1_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&zwp_fullscreen_shell_v1_interface, version, id);
	zwp_fullscreen_shell_v1_new(resource);
}

static void org_kde_kwin_server_decoration_manager_bind(struct wl_client
*client, void *data, uint32_t version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&org_kde_kwin_server_decoration_manager_interface, version, id);
	org_kde_kwin_server_decoration_manager_new(resource);
}

// For debugging
static bool global_filter(const struct wl_client *client, const struct wl_global
*global, void *data) {
	char *client_name = get_a_name((struct wl_client*)client);
	bool condition = wl_global_get_interface(global) == &zwp_fullscreen_shell_v1_interface;
	if (!strcmp(client_name, "weston-simple-d") && condition) {
		free(client_name);
		return false;
	}
	free(client_name);
	return true;
}

void create_globals(struct wl_display *D, bool dmabuf, void *user_data) {
	wl_list_init(&data_device_manager_list);

	wl_global_create(D, &wl_compositor_interface, 4, user_data,
	compositor_bind);
	wl_global_create(D, &wl_subcompositor_interface, 1, 0,
	subcompositor_bind);
	wl_global_create(D, &wl_data_device_manager_interface, 3, 0,
	data_device_manager_bind);
	wl_global_create(D, &wl_seat_interface, 5, user_data, seat_bind);
	wl_global_create(D, &wl_output_interface, 3, 0, output_bind);
	wl_display_init_shm(D);
	wl_global_create(D, &xdg_wm_base_interface, 1, user_data,
	xdg_wm_base_bind);
	if (dmabuf)
		wl_global_create(D, &zwp_linux_dmabuf_v1_interface, 3, user_data,
		 zwp_linux_dmabuf_v1_bind);
	wl_global_create(D, &zwp_linux_explicit_synchronization_v1_interface, 1,
	user_data, zwp_linux_explicit_synchronization_v1_bind);
	wl_global_create(D, &zwp_fullscreen_shell_v1_interface, 1, NULL,
	zwp_fullscreen_shell_v1_bind);
	wl_global_create(D, &org_kde_kwin_server_decoration_manager_interface,
	1, NULL, org_kde_kwin_server_decoration_manager_bind);

//	wl_display_set_global_filter(D, global_filter, 0);
}

#include <core/data_offer.h>

struct wl_client *focus = NULL;
struct wl_resource *selection = NULL;

static enum wl_iterator_result set_selection_all(struct wl_resource *resource,
void *user_data) {
	if (!strcmp(wl_resource_get_class(resource), "wl_data_device")) {
		struct wl_client *client = wl_resource_get_client(resource);
		struct wl_resource *source = user_data;
		if (selection) {
		struct wl_resource *offer = wl_resource_create(client,
		 &wl_data_offer_interface, 3, 0);
		wl_data_device_send_data_offer(resource, offer);
		struct data_offer *new = data_offer_new(offer, source);
		struct data_device *data = wl_resource_get_user_data(resource);
		wl_list_insert(&data->data_offer_list, &new->link);
		wl_data_device_send_selection(resource, offer);
		} else
		wl_data_device_send_selection(resource, NULL);
	}
	return WL_ITERATOR_CONTINUE;
}

static enum wl_iterator_result cancel(struct wl_resource *resource,
void *user_data) {
	if (!strcmp(wl_resource_get_class(resource), "wl_data_source")) {
		wl_data_source_send_cancelled(resource);
	}
	return WL_ITERATOR_CONTINUE;
}

void set_focused_client(struct wl_client *client) {
	focus = client;
/*	struct wl_display *display = wl_client_get_display(client);
	struct wl_list *list = wl_display_get_client_list(display);
	struct wl_client *client_;
	wl_client_for_each(client_, list) {
		wl_client_for_each_resource(client_, cancel, NULL);
	}*/

	wl_client_for_each_resource(focus, set_selection_all, selection);
}
