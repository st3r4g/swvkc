struct keyboard;
struct aaa;
struct wl_resource;

struct keyboard *keyboard_new(struct wl_resource *resource, struct input *input);
void keyboard_send(struct keyboard *keyboard, const struct aaa *aaa);
