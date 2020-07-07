struct data_device {
	struct wl_resource *resource;
	struct wl_list link;

	struct wl_list data_offer_list;
};

struct data_device *data_device_new(struct wl_resource *resource);
