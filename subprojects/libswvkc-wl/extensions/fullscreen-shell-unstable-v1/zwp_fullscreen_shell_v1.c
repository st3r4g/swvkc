#define _POSIX_C_SOURCE 200809L

#include <core/wl_surface.h>
#include <util/log.h>

#include <fullscreen-shell-unstable-v1-server-protocol.h>

static void release(struct wl_client *client, struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void present_surface(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *surface_resource, uint32_t method, struct
wl_resource *output) {
	errlog("PRESENT SURFACE");
	struct surface *surface = wl_resource_get_user_data(surface_resource);
	surface->role = ROLE_FULLSCREEN;
	surface->surface_events.map(surface, surface->surface_events.user_data);
	surface->is_mapped = true;
}

static void present_surface_for_mode(struct wl_client *client, struct
wl_resource *resource, struct wl_resource *surface, struct wl_resource *output,
int32_t framerate, uint32_t feedback) {
	errlog("PRESENT SURFACE FOR MODE");
}

static const struct zwp_fullscreen_shell_v1_interface impl = {release,
present_surface, present_surface_for_mode};

void zwp_fullscreen_shell_v1_new(struct wl_resource *resource) {
	errlog("BOUND TO FULLSCREEN SHELL");
	wl_resource_set_implementation(resource, &impl, NULL, NULL);

//	zwp_fullscreen_shell_v1_send_capability(resource, ZWP_FULLSCREEN_SHELL_V1_CAPABILITY_ARBITRARY_MODES);
//	zwp_fullscreen_shell_v1_send_capability(resource, ZWP_FULLSCREEN_SHELL_V1_CAPABILITY_CURSOR_PLANE);
}
