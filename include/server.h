#ifndef MYSERVER_H
#define MYSERVER_H

struct server;
struct xdg_toplevel_data;
struct xdg_surface0;

void server_request_redraw(struct server *server);
void server_window_create(struct xdg_toplevel_data *);
void server_window_destroy(struct xdg_toplevel_data *);
void server_set_focus(struct xdg_toplevel_data *);
int server_surface_is_focused(struct xdg_surface0 *);
struct screen *server_get_screen(struct server *server);

#endif
