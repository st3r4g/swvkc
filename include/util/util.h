struct wl_client;

struct wl_resource *util_wl_client_get_keyboard(struct wl_client *);
char *read_file(const char *);
char *get_a_name(struct wl_client *client);
void dmabuf_save_to_disk(int fd);
