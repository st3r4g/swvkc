struct mime_node {
	char *mime_type;
	struct wl_list link;
};

struct offer_node {
	struct wl_resource *offer;
	struct wl_list link;
};

struct data_source {
	struct wl_list mime_list;
	struct wl_list offer_list;
};

void data_source_new(struct wl_resource *resource);
void data_source_bind_offer(struct wl_resource *resource, struct wl_resource
*offer);
