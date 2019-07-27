struct wli_subsurface {
	struct wl_resource *surface;
	struct wl_resource *parent;
};

void wl_subsurface_new(struct wl_resource *subsurface, struct wl_resource
*surface, struct wl_resource *parent);
