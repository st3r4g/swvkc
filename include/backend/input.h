#include <stdint.h>

struct input;

struct input *input_setup();
int input_get_key_fd(struct input *S);
int input_get_poi_fd(struct input *S);
unsigned int input_get_keymap_fd(struct input *S);
unsigned int input_get_keymap_size(struct input *S);

struct aaa {
	unsigned int key;
	unsigned int state;
	char name[2];

	unsigned int mods_depressed;
	unsigned int mods_latched;
	unsigned int mods_locked;
	unsigned int group;
};

struct pointer {
	int32_t x;
	int32_t y;
} pointer;

int input_handle_event(struct input *S, struct aaa *aaa, int fd);
void input_release(struct input *S);
