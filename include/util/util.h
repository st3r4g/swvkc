struct wl_client;

struct extension_node {
	struct wl_resource *resource;
	struct wl_list link;
};

struct wl_resource *util_wl_client_get_keyboard(struct wl_client *);
struct wl_resource *util_wl_client_get_output(struct wl_client *client);
struct wl_resource *util_get_extension(struct wl_list *extensions, const char
*class);
char *read_file(const char *);
char *get_a_name(struct wl_client *client);
void dmabuf_save_to_disk(int fd);
int a();
void fd_test(int n);
