struct input;

struct input *input_setup();
int input_get_key_fd(struct input *S);
unsigned int input_get_keymap_fd(struct input *S);
unsigned int input_get_keymap_size(struct input *S);

struct aaa {
	unsigned int key;
	unsigned int state;

	unsigned int mods_depressed;
	unsigned int mods_latched;
	unsigned int mods_locked;
	unsigned int group;
};

_Bool input_handle_event(struct input *S, struct aaa *aaa);
void input_release(struct input *S);
