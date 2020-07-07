struct data_device_manager {
	struct wl_resource *resource;
	struct wl_list link;

	struct wl_list data_device_list;
	struct wl_list data_source_list;
};

struct data_device_manager * data_device_manager_new(struct wl_resource *resource);
