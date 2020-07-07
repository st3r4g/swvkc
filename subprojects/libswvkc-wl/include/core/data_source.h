struct mime_node {
	char *mime_type;
	struct wl_list link;
};

struct data_source {
	struct wl_resource *resource;
	struct wl_list link;

	struct wl_list list;
};

struct data_source *data_source_new(struct wl_resource *resource);
