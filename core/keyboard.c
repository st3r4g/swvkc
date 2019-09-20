#define _POSIX_C_SOURCE 200809L
#include <util/log.h>
#include <core/keyboard.h>

#include <stdlib.h>
#include <wayland-server-protocol.h>

struct keyboard {
	struct wl_resource *resource;

	struct keyboard_events keyboard_events;
};

static void keyboard_release(struct wl_client *client, struct wl_resource
*resource) {
	wl_resource_destroy(resource);
}

static const struct wl_keyboard_interface impl = {
	.release = keyboard_release
};

void keyboard_new(struct wl_resource *resource, struct
keyboard_events keyboard_events) {
	struct keyboard *keyboard = malloc(sizeof(struct keyboard));
	keyboard->resource = resource;
	keyboard->keyboard_events = keyboard_events;

	wl_resource_set_implementation(resource, &impl, keyboard, 0);

	if (wl_resource_get_version(resource) >= WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION)
		wl_keyboard_send_repeat_info(resource, 30, 300);

	keyboard_events.init(keyboard, keyboard_events.user_data);
}

void keyboard_send_keymap(struct keyboard *keyboard, int32_t fd, uint32_t size) {
	struct wl_resource *resource = keyboard->resource;
	if (wl_resource_get_version(resource) >= WL_KEYBOARD_KEYMAP_SINCE_VERSION)
		wl_keyboard_send_keymap(resource,
		 WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd, size);
}

void keyboard_send_key(struct keyboard *keyboard, uint32_t key, uint32_t state) {
	struct wl_resource *resource = keyboard->resource;
	if (wl_resource_get_version(resource) >= WL_KEYBOARD_KEY_SINCE_VERSION)
		wl_keyboard_send_key(resource, 0, 0, key, state);
}

void keyboard_send_modifiers(struct keyboard *keyboard, uint32_t mods_depressed,
uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
	struct wl_resource *resource = keyboard->resource;
	if (wl_resource_get_version(resource) >= WL_KEYBOARD_MODIFIERS_SINCE_VERSION)
		wl_keyboard_send_modifiers(resource, 0, mods_depressed,
		 mods_latched, mods_locked, group);
}
