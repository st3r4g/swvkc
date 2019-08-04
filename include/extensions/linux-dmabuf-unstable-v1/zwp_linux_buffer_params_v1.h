#include <wayland-server-core.h>

#include <stdbool.h>

struct plane {
	int32_t fd;
	uint32_t plane_idx;
	uint32_t offset;
	uint32_t stride;
	uint64_t modifier;

	struct wl_list link;
};

struct zwp_linux_buffer_params_v1_data {
	bool already_used;
	struct wl_list plane_list;
};

struct zwp_linux_buffer_params_v1_data *zwp_linux_buffer_params_v1_new(struct
wl_resource *);
