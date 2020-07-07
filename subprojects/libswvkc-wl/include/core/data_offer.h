struct data_offer {
	struct wl_resource *resource;
	struct wl_list link;

	struct wl_resource *source;
};

struct data_offer *data_offer_new(struct wl_resource *resource, struct wl_resource *source);
