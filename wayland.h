#ifndef wayland_h_INCLUDED
#define wayland_h_INCLUDED

void wayland_init();
int wayland_get_fd();
void wayland_read();
void wayland_send_key(uint32_t key, uint32_t state);
void wayland_flush();

#endif // wayland_h_INCLUDED

