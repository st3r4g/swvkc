#include <stdbool.h>

struct data_offer {
	struct wl_resource *source;
	bool valid;
};

void data_offer_new(struct wl_resource *resource, struct wl_resource *source);
void data_offer_invalidate(struct wl_resource *resource);
