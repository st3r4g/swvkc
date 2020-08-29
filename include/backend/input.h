struct aaa {
	unsigned int key;
	unsigned int state;
	char name[2];

	unsigned int mods_depressed;
	unsigned int mods_latched;
	unsigned int mods_locked;
	unsigned int group;
};

typedef void (*input_key_event_t)(struct aaa *, void *);
typedef void (*input_modifers_event_t)(struct aaa *, void *);
typedef void (*input_motion_event_t)(unsigned int, int, int, void *);
typedef void (*input_frame_event_t)(void *);
typedef void (*input_button_event_t)(unsigned int, unsigned int, unsigned int,
void *);

struct input_events {
	input_key_event_t key;
	input_modifers_event_t modifiers;
	input_motion_event_t motion;
	input_frame_event_t frame;
	input_button_event_t button;

	void *user_data;
};

struct input;

struct input *input_setup(char *kdevpath, char *pdevpath, struct input_events input_events);
int input_get_key_fd(struct input *S);
int input_get_poi_fd_n(struct input *S);
int input_get_poi_fd(struct input *S, int i);
unsigned int input_get_keymap_fd(struct input *S);
unsigned int input_get_keymap_size(struct input *S);
void input_release(struct input *S);
