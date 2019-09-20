#include <stdint.h>

struct keyboard;
struct wl_resource;
typedef void (*keyboard_init_t)(struct keyboard *, void *);

struct keyboard_events {
	keyboard_init_t init;

	void *user_data;
};

void keyboard_new(struct wl_resource *resource, struct keyboard_events
keyboard_events);
void keyboard_send_keymap(struct keyboard *keyboard, int32_t fd, uint32_t size);
void keyboard_send_key(struct keyboard *keyboard, uint32_t key, uint32_t state);
void keyboard_send_modifiers(struct keyboard *keyboard, uint32_t mods_depressed,
uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
