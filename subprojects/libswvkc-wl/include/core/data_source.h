struct mime_node {
	char *mime_type;
	struct wl_list link;
};

void data_source_new(struct wl_resource *resource);
