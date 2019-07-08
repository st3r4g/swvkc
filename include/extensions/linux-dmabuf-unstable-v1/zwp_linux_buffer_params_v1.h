struct zwp_linux_buffer_params_v1_data {
	int32_t fd;
	uint32_t plane_idx;
	uint32_t offset;
	uint32_t stride;
	uint64_t modifier;
};

struct zwp_linux_buffer_params_v1_data *zwp_linux_buffer_params_v1_new(struct
wl_resource *);
