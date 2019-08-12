#define _POSIX_C_SOURCE 200809L
#include <backend/input.h>
#include <util/log.h>
#include <core/keyboard.h>
#include <extensions/xdg_shell.h>

#include <stdlib.h>
#include <wayland-server-protocol.h>

struct keyboard {
	struct wl_resource *resource;
	struct input *input;
};

static void keyboard_release(struct wl_client *client, struct wl_resource
*resource) {
}

static const struct wl_keyboard_interface impl = {
	.release = keyboard_release
};

struct keyboard *keyboard_new(struct wl_resource *resource, struct input *input) {
	struct keyboard *keyboard = calloc(1, sizeof(struct keyboard));
	keyboard->resource = resource;
	keyboard->input = input;
	wl_resource_set_implementation(resource, &impl, keyboard, 0);

	if (wl_resource_get_version(resource) >= WL_KEYBOARD_KEYMAP_SINCE_VERSION)
		wl_keyboard_send_keymap(resource,
		WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, input_get_keymap_fd(input),
		input_get_keymap_size(input));
	if (wl_resource_get_version(resource) >= WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION)
		wl_keyboard_send_repeat_info(resource, 30, 300);

	return keyboard;
}

void keyboard_send(struct keyboard *keyboard, const struct aaa *aaa) {
	struct wl_resource *resource = keyboard->resource;
	if (wl_resource_get_version(resource) >= WL_KEYBOARD_KEY_SINCE_VERSION)
		wl_keyboard_send_key(resource, 0, 0, aaa->key, aaa->state);
	if (wl_resource_get_version(resource) >= WL_KEYBOARD_MODIFIERS_SINCE_VERSION)
		wl_keyboard_send_modifiers(resource, 0, aaa->mods_depressed,
		aaa->mods_latched, aaa->mods_locked, aaa->group);
}
